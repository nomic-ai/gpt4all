import gpt from '../src/gpt4all.js'

const model = await gpt.loadModel('mistral-7b-openorca.Q4_0.gguf', { verbose: true, device: 'gpu' });


process.stdout.write('Response: ')

const stream = gpt.createTokenStream(model,[{
    role: 'user',
    content: "How are you ?"
  }],{ nPredict: 2048 })

stream.pipe(process.stdout)

await new Promise((res) => {
    let s = stream
    stream.on('finish', () => { 
        res(s);   
    })
});

const result = await gpt.createCompletion(model, [{
    role: 'user',
    content: "Are you sure you're okay?"
}])

console.log(result)

model.dispose();

