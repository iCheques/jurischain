'use strict';

/**
 * JurisChain Node.js Native Addon - Integration Test
 */

const { Jurischain } = require('../bindings/node');

console.log('=== Node.js Addon Test ===');
console.log(`Node Version: ${process.version}\n`);

// Test 1: Constructor
console.log('[1] Testing constructor... ');
const challenge = new Jurischain(10, 'TestSeed');
console.log('OK');

// Test 2: readChallenge
console.log('[2] Testing readChallenge... ');
const hash = challenge.readChallenge();
if (typeof hash !== 'string' || hash.length !== 64) {
  console.error(`FAIL: expected 64-char hex string, got "${hash}" (len=${hash.length})`);
  process.exit(1);
}
console.log(`OK (hash=${hash})`);

// Test 3: solveStep (solve the challenge)
console.log('[3] Testing solveStep (solving)... ');
let tries = 0;
while (!challenge.solveStep()) {
  tries++;
  if (tries > 1000000) {
    console.error('FAIL: exceeded 1M tries');
    process.exit(1);
  }
}
tries++;
console.log(`OK (solved in ${tries} tries)`);

// Test 4: verify
console.log('[4] Testing verify... ');
const valid = challenge.verify();
if (valid !== true) {
  console.error(`FAIL: expected true, got ${valid}`);
  process.exit(1);
}
console.log('OK');

// Test 5: readChallenge (solution)
console.log('[5] Testing readChallenge (solution)... ');
const solution = challenge.readChallenge();
if (typeof solution !== 'string' || solution.length !== 64) {
  console.error(`FAIL: expected 64-char hex string, got "${solution}"`);
  process.exit(1);
}
console.log(`OK (solution=${solution})`);

// Test 6: challengeResponse (transfer solution)
console.log('[6] Testing challengeResponse + verify... ');
const challenge2 = new Jurischain(10, 'TestSeed');
const setResult = challenge2.challengeResponse(solution);
if (setResult !== true) {
  console.error(`FAIL: challengeResponse returned ${setResult}`);
  process.exit(1);
}
const verify2 = challenge2.verify();
if (verify2 !== true) {
  console.error(`FAIL: verify after challengeResponse returned ${verify2}`);
  process.exit(1);
}
console.log('OK');

// Test 7: Input validation - difficulty range
console.log('[7] Testing difficulty validation... ');
try {
  new Jurischain(0, 'TestSeed');
  console.error('FAIL: should have thrown');
  process.exit(1);
} catch (e) {
  console.log(`OK (caught: ${e.message})`);
}

// Test 8: Input validation - empty seed
console.log('[8] Testing empty seed validation... ');
try {
  new Jurischain(10, '');
  console.error('FAIL: should have thrown');
  process.exit(1);
} catch (e) {
  console.log(`OK (caught: ${e.message})`);
}

// Test 9: Input validation - non-finite & non-integer difficulty (no C++ UB)
console.log('[9] Testing NaN / Infinity / float / out-of-range difficulty... ');
for (const badDiff of [NaN, Infinity, -Infinity, 1.5, -1, 256, 1e20]) {
  try {
    new Jurischain(badDiff, 'TestSeed');
    console.error(`FAIL: should have thrown for difficulty=${badDiff}`);
    process.exit(1);
  } catch (e) {
    // expected
  }
}
console.log('OK');

// Test 10: Seed max length DoS protection (ASCII and multi-byte UTF-8)
console.log('[10] Testing seed max length validation... ');
for (const badSeed of ['A'.repeat(1025), '\u00E9'.repeat(600)]) {
  try {
    new Jurischain(10, badSeed);
    console.error('FAIL: should have thrown for oversized seed');
    process.exit(1);
  } catch (e) {
    // expected
  }
}
console.log('OK');

// Test 11: challengeResponse rejects whitespace, signs, prefixes & invalid hex without corrupting state
console.log('[11] Testing challengeResponse strict hex validation & atomicity... ');
const atomicCheck = new Jurischain(10, 'TestSeed');
atomicCheck.challengeResponse(solution);
for (const badHex of [
  ' F' + '00'.repeat(31),
  '+1' + '00'.repeat(31),
  '-1' + '00'.repeat(31),
  '0x' + '00'.repeat(31),
  'GG' + '00'.repeat(31),
  'ABCD',
]) {
  let threw = false;
  try {
    atomicCheck.challengeResponse(badHex);
  } catch (e) {
    threw = true;
  }
  if (!threw) {
    console.error(`FAIL: challengeResponse accepted invalid hex "${badHex}"`);
    process.exit(1);
  }
}
if (atomicCheck.verify() !== true) {
  console.error('FAIL: failed challengeResponse corrupted previously valid state');
  process.exit(1);
}
console.log('OK');

// Test 12: challengeResponse accepts lowercase hex
console.log('[12] Testing challengeResponse accepts lowercase hex... ');
const lc = new Jurischain(10, 'TestSeed');
lc.challengeResponse(solution.toLowerCase());
if (lc.verify() !== true) {
  console.error('FAIL: lowercase hex should verify');
  process.exit(1);
}
console.log('OK');

// Test 13: Invalid `this` receiver throws TypeError instead of crashing
console.log('[13] Testing invalid receiver guard... ');
for (const method of ['verify', 'solveStep', 'readChallenge', 'challengeResponse']) {
  let threw = false;
  try {
    Jurischain.prototype[method].call({}, solution);
  } catch (e) {
    threw = e instanceof TypeError;
  }
  if (!threw) {
    console.error(`FAIL: ${method} did not throw TypeError on invalid receiver`);
    process.exit(1);
  }
}
console.log('OK');

// Test 14: Tamper rejection (wrong seed & wrong difficulty)
console.log('[14] Testing tamper rejection... ');
const wrongSeed = new Jurischain(10, 'OtherSeed');
wrongSeed.challengeResponse(solution);
if (wrongSeed.verify() !== false) {
  console.error('FAIL: wrong seed verified');
  process.exit(1);
}
const wrongDiff = new Jurischain(24, 'TestSeed');
wrongDiff.challengeResponse(solution);
if (wrongDiff.verify() !== false) {
  console.error('FAIL: wrong difficulty verified');
  process.exit(1);
}
console.log('OK');

// Test 15: Worker thread safety (context-aware addon)
console.log('[15] Testing worker_threads compatibility... ');
const { Worker } = require('worker_threads');
const path = require('path');
const bindingPath = path.resolve(__dirname, '../bindings/node');
const worker = new Worker(`
  const { parentPort, workerData } = require('worker_threads');
  const { Jurischain } = require(workerData.bindingPath);
  const c = new Jurischain(10, 'TestSeed');
  while (!c.solveStep());
  parentPort.postMessage(c.readChallenge());
`, { eval: true, workerData: { bindingPath } });

worker.on('message', (workerSolution) => {
  if (workerSolution !== solution) {
    console.error(`FAIL: worker solution mismatch: ${workerSolution} !== ${solution}`);
    process.exit(1);
  }
  console.log('OK');
  console.log('\n=== All Node.js tests passed! ===');
});
worker.on('error', (err) => {
  console.error('FAIL: worker threw error:', err);
  process.exit(1);
});
