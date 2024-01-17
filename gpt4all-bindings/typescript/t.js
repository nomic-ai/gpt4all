const gpt = require('./src/gpt4all')

async function main(){
  const model = await gpt.loadModel("mistral-7b-openorca.Q4_0.gguf")  

  process.stdout.write('Response: ')

  const stream = gpt.createTokenStream(model,[{
    role: 'user',
    content: "How are you ?"
  }],{nPredict: 2048 }).pipe(process.stdout)

  return new Promise((res) => {
    stream.on('end',() => {
        res()
    })
  })
}

main()