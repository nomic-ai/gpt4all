import * as assert from 'node:assert'
import { download } from '../src/gpt4all.js'


assert.rejects(async () => download('poo.bin').promise());


console.log('OK')
