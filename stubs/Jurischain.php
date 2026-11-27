<?php
/**
 * @credithub/jurischain — PHP Extension Stub
 *
 * This file provides IDE autocompletion and type information for the
 * Jurischain C extension. It is NOT loaded at runtime.
 *
 * @package Jurischain
 * @version 1.1.3
 */

/**
 * SHA3-256 Proof-of-Work challenge solver and verifier.
 *
 * Generates a computational challenge based on a seed and difficulty level.
 * The client iterates SHA3-256 hashes until the result meets the difficulty
 * threshold, then the server can verify the solution in O(1).
 *
 * @example
 * ```php
 * // Client-side: generate and solve
 * $challenge = new Jurischain(10, random_bytes(32));
 * while (!$challenge->solve());
 * $hash = $challenge->getChallenge();
 *
 * // Server-side: verify submitted solution
 * $verifier = new Jurischain(10, $originalSeed);
 * $verifier->setResponse($_POST['jurischain']);
 * $valid = $verifier->verify(); // true
 * ```
 */
final class Jurischain
{
    /**
     * Create a new Proof-of-Work challenge.
     *
     * @param int    $difficulty Difficulty level (1–255). Higher values require
     *                           exponentially more computation to solve.
     * @param string $seed       Non-empty seed string. Must be identical on both
     *                           the solver and verifier sides.
     *
     * @throws \ValueError If difficulty is outside 1–255 range.
     * @throws \ValueError If seed is an empty string or exceeds 1024 bytes.
     */
    public function __construct(int $difficulty, string $seed) {}

    /**
     * Attempt one solve iteration of the PoW challenge.
     *
     * Call this in a loop until it returns `true`. Each call performs a single
     * SHA3-256 hash attempt and increments the internal nonce.
     *
     * @return bool `true` if the challenge is solved, `false` otherwise.
     *
     * @example
     * ```php
     * while (!$challenge->solve());
     * ```
     */
    public function solve(): bool { return false; }

    /**
     * Verify that the current state contains a valid solution.
     *
     * After solving (client-side) or after calling `setResponse()` (server-side),
     * this method checks whether the hash meets the difficulty threshold.
     *
     * @return bool `true` if the solution is valid, `false` otherwise.
     */
    public function verify(): bool { return false; }

    /**
     * Read the current challenge or solution hash as a hex string.
     *
     * Before solving, returns the initial challenge hash (SHA3-256 of the seed).
     * After solving, returns the solution hash.
     *
     * @return string 64-character uppercase hex string (32 bytes).
     */
    public function getChallenge(): string { return ''; }

    /**
     * Set the response hash from a client submission for server-side verification.
     *
     * Used on the server to inject a solution received from the client before
     * calling `verify()`.
     *
     * @param string $response 64-character hex string (32 bytes) — the solution
     *                         hash returned by `getChallenge()` on the solver side.
     *
     * @return bool `true` on success.
     *
     * @throws \ValueError If response is not exactly 64 hex characters.
     * @throws \ValueError If response contains invalid hex characters.
     */
    public function setResponse(string $response): bool { return false; }
}
