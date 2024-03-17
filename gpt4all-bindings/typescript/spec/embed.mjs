import { loadModel, createEmbedding } from '../src/gpt4all.js'

const embedder = await loadModel("nomic-embed-text-v1.5.f16.gguf", { verbose: true, type: 'embedding'})

console.log(createEmbedding(embedder, ["Accept your current situation"]))

