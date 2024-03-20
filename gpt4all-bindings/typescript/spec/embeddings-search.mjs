import { loadModel, createEmbedding } from '../src/gpt4all.js'
import { createGunzip, createGzip, createUnzip } from 'node:zlib';
import { Readable } from 'stream'
import readline  from 'readline'
const embedder = await loadModel("nomic-embed-text-v1.5.f16.gguf", { verbose: true, type: 'embedding' })
console.log("Running with", embedder.llm.threadCount(), "threads");

function cosineDistance(a, b) {
    let dotProduct = 0;
    let magA = 0;
    let magB = 0;
    for (let i = 0; i < a.length; i++) {
        dotProduct += a[i] * b[i];
        magA += a[i] * a[i];
        magB += b[i] * b[i];
    }
    return 1 - (dotProduct / (Math.sqrt(magA) * Math.sqrt(magB)));
}

const unzip = createGunzip();
const url = "https://huggingface.co/datasets/sentence-transformers/embedding-training-data/resolve/main/squad_pairs.jsonl.gz"
const stream = await fetch(url)
        .then(res => Readable.fromWeb(res.body));

const lineReader = readline.createInterface({ 
    input: stream.pipe(unzip),
    crlfDelay: Infinity
})

lineReader.on('line', line => {
    //pairs of questions and answers
    const question_answer = JSON.parse(line)
    console.log("embedding", question_answer)
    console.log(createEmbedding(embedder, question_answer))

})



