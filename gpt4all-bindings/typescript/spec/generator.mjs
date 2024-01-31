import gpt from '../src/gpt4all.js'

const model = await gpt.loadModel("mistral-7b-openorca.Q4_0.gguf")  

process.stdout.write('Response: ')

for await (const token of gpt.generateTokens(model,[{
    role: 'user',
    content: "How are you ?"
  }],{nPredict: 2048 })){
    process.stdout.write(token);
  }

model.dispose();

