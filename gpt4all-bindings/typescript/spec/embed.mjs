import { loadModel, createEmbedding } from '../src/gpt4all.js'

const embedder = await loadModel("all-MiniLM-L6-v2-f16.gguf", { verbose: true, type: 'embedding'})

console.log(createEmbedding(embedder, "Accept your current situation"))

