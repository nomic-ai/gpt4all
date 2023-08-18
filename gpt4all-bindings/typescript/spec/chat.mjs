import { LLModel, createCompletion, DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY, loadModel } from '../src/gpt4all.js'

const model = await loadModel(
    'orca-mini-3b.ggmlv3.q4_0.bin',
    { verbose: true }
);
const ll = model.llm;

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

const completion1 = await createCompletion(model, [ 
    { role : 'system', content: 'You are an advanced mathematician.'  },
    { role : 'user', content: 'What is 1 + 1?'  }, 
], { verbose: true })
console.log(completion1.choices[0].message)

const completion2 = await createCompletion(model, [
    { role : 'system', content: 'You are an advanced mathematician.'  },
    { role : 'user', content: 'What is two plus two?'  }, 
], {  verbose: true })

console.log(completion2.choices[0].message)

// At the moment, from testing this code, concurrent model prompting is not possible. 
// Behavior: The last prompt gets answered, but the rest are cancelled
// my experience with threading is not the best, so if anyone who is good is willing to give this a shot,
// maybe this is possible
// INFO: threading with llama.cpp is not the best maybe not even possible, so this will be left here as reference

//const responses = await Promise.all([
//    createCompletion(ll, [ 
//    { role : 'system', content: 'You are an advanced mathematician.'  },
//    { role : 'user', content: 'What is 1 + 1?'  }, 
//    ], { verbose: true }),
//    createCompletion(ll, [ 
//    { role : 'system', content: 'You are an advanced mathematician.'  },
//    { role : 'user', content: 'What is 1 + 1?'  }, 
//    ], { verbose: true }),
//
//createCompletion(ll, [ 
//    { role : 'system', content: 'You are an advanced mathematician.'  },
//    { role : 'user', content: 'What is 1 + 1?'  }, 
//], { verbose: true })
//
//])
//console.log(responses.map(s => s.choices[0].message))

