import gpt from '../src/gpt4all.js'

const model = await gpt.loadModel("mistral-7b-openorca.Q4_0.gguf", { device: 'gpu' })  

process.stdout.write('Response: ')


const tokens = gpt.generateTokens(model, [{ 
    role: 'user',
    content: "How are you ?"
}], { nPredict: 2048 })
for await (const token of tokens){
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


const tokens2 = gpt.generateTokens(model, [{ 
    role: 'user',
    content: "If 3 + 3 is 5, what is 2 + 2?"
}], { nPredict: 2048 })
for await (const token of tokens2){
    process.stdout.write(token);
}
console.log("done")
model.dispose();

