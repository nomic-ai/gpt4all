import gpt from '../src/gpt4all.js'

const model = await gpt.loadModel("mistral-7b-openorca.Q4_0.gguf")  

process.stdout.write('Response: ')

for await (const token of gpt.generateTokens(model,[{
    role: 'user',
    content: "How are you ?"
  }],{nPredict: 2048 })){
    process.stdout.write(token);
  }


const result = await gpt.createCompletion(model, [{ 
    role: 'user',
    content: "You sure?"
}])

console.log(result)


const result2 = await gpt.createCompletion(model, [{ 
    role: 'user',
    content: "You sure you sure?"
}])

console.log(result2)
model.dispose();

