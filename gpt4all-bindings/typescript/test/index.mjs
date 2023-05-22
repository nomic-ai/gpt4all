import * as assert from 'node:assert'
import { prompt, download } from '../src/gpt4all.js'

{

    const somePrompt = prompt`${"header"} Hello joe, my name is Ron. ${"prompt"}`;
    assert.equal(
        somePrompt({ header: 'oompa', prompt: 'holy moly' }),   
        'oompa Hello joe, my name is Ron. holy moly'
    );

}

{

    const indexedPrompt = prompt`${0}, ${1} ${0}`;
    assert.equal(
        indexedPrompt('hello', 'world'),
        'hello, world hello'
    );
    
    assert.notEqual(
        indexedPrompt(['hello', 'world']),
        'hello, world hello'
    );

}

{
    assert.equal(
    (prompt`${"header"} ${"prompt"}`)({ header: 'hello', prompt: 'poo' }), 'hello poo',
    "Template prompt not equal"
    );

}


assert.rejects(async () => download('poo.bin').promise());


console.log('OK')
