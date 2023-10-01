import os
import openai
from langchain.vectorstores import Chroma
from langchain.embeddings import HuggingFaceInstructEmbeddings, HuggingFaceEmbeddings, HuggingFaceBgeEmbeddings
from chromadb.config import Settings
import yaml
import torch

ROOT_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
SOURCE_DIRECTORY = f"{ROOT_DIRECTORY}/Docs_for_DB"
PERSIST_DIRECTORY = f"{ROOT_DIRECTORY}/Vector_DB"
INGEST_THREADS = os.cpu_count() or 8

CHROMA_SETTINGS = Settings(
    chroma_db_impl="duckdb+parquet", persist_directory=PERSIST_DIRECTORY, anonymized_telemetry=False
)

openai.api_base = 'http://localhost:4891/v1'
openai.api_key = ''

prefix = ""
suffix = ""

def connect_to_local_chatgpt(prompt):
    formatted_prompt = f"{prefix}{prompt}{suffix}"
    response = openai.ChatCompletion.create(
        model="local model",
        temperature=0.1,
        messages=[{"role": "user", "content": formatted_prompt}]
    )
    return response.choices[0].message["content"]

def ask_local_chatgpt(query, persist_directory=PERSIST_DIRECTORY, client_settings=CHROMA_SETTINGS):
    with open('config.yaml', 'r') as config_file:
        config = yaml.safe_load(config_file)
        EMBEDDING_MODEL_NAME = config['EMBEDDING_MODEL_NAME']
        COMPUTE_DEVICE = config.get("COMPUTE_DEVICE", "cpu")

    if "instructor" in EMBEDDING_MODEL_NAME:
        embeddings = HuggingFaceInstructEmbeddings(
            model_name=EMBEDDING_MODEL_NAME,
            model_kwargs={"device": COMPUTE_DEVICE},
            encode_kwargs={'normalize_embeddings': True}
        )
    elif "bge" in EMBEDDING_MODEL_NAME and "large-en-v1.5" not in EMBEDDING_MODEL_NAME:
        embeddings = HuggingFaceBgeEmbeddings(
            model_name=EMBEDDING_MODEL_NAME,
            model_kwargs={"device": COMPUTE_DEVICE},
            encode_kwargs={'normalize_embeddings': True}
        )
    else:
        embeddings = HuggingFaceEmbeddings(
            model_name=EMBEDDING_MODEL_NAME,
            model_kwargs={'device': COMPUTE_DEVICE},
            encode_kwargs={'normalize_embeddings': True}
        )

    db = Chroma(
        persist_directory=persist_directory,
        embedding_function=embeddings,
        client_settings=client_settings,
    )
    retriever = db.as_retriever()
    relevant_contexts = retriever.get_relevant_documents(query)
    contexts = [document.page_content for document in relevant_contexts]
    prepend_string = "Only base your answer to the following question on the provided context.  If the provided context does not provide an answer, simply state that is the case."
    augmented_query = "\n\n---\n\n".join(contexts) + "\n\n-----\n\n" + query
    response_json = connect_to_local_chatgpt(augmented_query)
    
    with open("relevant_contexts.txt", "w", encoding="utf-8") as f:
        for content in contexts:
            f.write(content + "\n\n---\n\n")
        
    return {"answer": response_json, "sources": relevant_contexts}

def interact_with_chat(user_input):
    global last_response
    response = ask_local_chatgpt(user_input)
    answer = response['answer']
    last_response = answer
    return answer