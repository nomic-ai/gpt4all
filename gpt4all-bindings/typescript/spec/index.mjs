import { LLModel, createCompletion, DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY, loadModel } from '../src/gpt4all.js'

const ll = await loadModel(
    'ggml-gpt4all-j-v1.3-groovy.bin',
    { verbose: true }
);

try {
   class Extended extends LLModel {
   }

} catch(e) {
    console.log("Extending from native class gone wrong " + e)
}

console.log("state size " + ll.stateSize())

console.log("thread count " + ll.threadCount());
ll.setThreadCount(5);

console.log("thread count " + ll.threadCount());
ll.setThreadCount(4);
console.log("thread count " + ll.threadCount());
console.log("name " + ll.name());
console.log("type: " + ll.type());
console.log("Default directory for models", DEFAULT_DIRECTORY);
console.log("Default directory for libraries", DEFAULT_LIBRARIES_DIRECTORY);

const completion1 = await createCompletion(ll, [ 
    { role : 'system', content: 'You are an advanced mathematician.'  },
    { role : 'user', content: 'What is 1 + 1?'  }, 
], { verbose: true })
console.log(completion1.choices[0].message)

const completion2 = await createCompletion(ll, [
    { role : 'system', content: 'You are an advanced mathematician.'  },
    { role : 'user', content: 'What is two plus two?'  }, 
], {  verbose: true })

 console.log(completion2.choices[0].message)
