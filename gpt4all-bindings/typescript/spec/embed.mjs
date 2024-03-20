import { loadModel, createEmbedding } from '../src/gpt4all.js'

const embedder = await loadModel("nomic-embed-text-v1.5.f16.gguf", { verbose: true, type: 'embedding' , device: 'gpu' })

try {
console.log(createEmbedding(embedder, ["Accept your current situation", "12312"], { prefix: "search_document"  }))

} catch(e) {
console.log(e)
}

embedder.dispose()
