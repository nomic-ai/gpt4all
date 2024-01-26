import gpt from '../src/gpt4all.js'

const model = await gpt.loadModel("mistral-7b-openorca.Q4_0.gguf")  

process.stdout.write('Response: ')

const stream = gpt.createTokenStream(model,[{
    role: 'user',
    content: "How are you ?"
  }],{nPredict: 2048 }).pipe(process.stdout)

await new Promise((res) => {
    stream.on('end', () => { 
        res();   
    })
});

await gpt.createCompletion(model, [{
    role: 'user',
    content: "Are you sure you're okay?"
}])

model.dispose();

