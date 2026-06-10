--TEST--
Jurischain: generate, solve, transfer and verify a PoW challenge (OOP API)
--SKIPIF--
<?php if (!extension_loaded("jurischain")) print "skip jurischain extension not loaded"; ?>
--FILE--
<?php
$difficulty = 10;
$seed = "Hello";

/* Client side: generate a challenge and solve it. */
$challenge = new Jurischain($difficulty, $seed);
while (!$challenge->solve());
$solution = $challenge->getChallenge();

var_dump(strlen($solution) === 64);
var_dump($challenge->verify());

/* Server side: rebuild from the same seed, inject the client's solution. */
$verifier = new Jurischain($difficulty, $seed);
$verifier->setResponse($solution);
var_dump($verifier->verify());

/* A forged solution from a different seed must not verify. */
$wrong = new Jurischain($difficulty, "Goodbye");
$wrong->setResponse($solution);
var_dump($wrong->verify());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
