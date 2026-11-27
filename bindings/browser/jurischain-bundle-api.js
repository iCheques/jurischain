
/**
 * Promise API for JurisChain PoW — bundled version.
 * This file is concatenated AFTER the modularized ASM.js output
 * inside an IIFE by the build system. _JurischainASM is available
 * in the same closure scope.
 *
 * Calls are serialized: if a solve is already in progress, subsequent
 * calls wait in a queue and execute one at a time.
 *
 * @param {Object}  options
 * @param {string}  options.seed       - Random seed for the challenge
 * @param {number}  options.difficulty  - Difficulty level (1-255)
 * @param {number}  [options.timeout]  - Optional timeout in ms (0 = no timeout)
 * @returns {Promise<string>} The hex-encoded solution hash
 */
var _solveQueue = Promise.resolve();

function _solve(seed, diff, timeout) {
  return new Promise(function (resolve, reject) {
    var settled = false;
    var timerId;
    var modInstance = null;

    window.jurischain = Object.freeze({
      seed: seed,
      difficulty: String(diff),
    });

    function cancelModule() {
      if (modInstance && typeof modInstance._cancel_solve === 'function') {
        try {
          modInstance._cancel_solve();
        } catch (e) {
          // Ignore cancellation errors during teardown
        }
      }
    }

    function onSolved(event) {
      if (settled) return;
      settled = true;
      cleanup();
      resolve(event.detail);
    }

    function cleanup() {
      document.removeEventListener('jurischain', onSolved);
      if (timerId) clearTimeout(timerId);
    }

    document.addEventListener('jurischain', onSolved);

    if (timeout > 0) {
      timerId = setTimeout(function () {
        if (settled) return;
        settled = true;
        cleanup();
        cancelModule();
        reject(new Error('JurisChain solver timed out after ' + timeout + 'ms'));
      }, timeout);
    }

    try {
      _JurischainASM().then(function (mod) {
        modInstance = mod;
        if (settled) {
          cancelModule();
        }
      }, function (err) {
        if (settled) return;
        settled = true;
        cleanup();
        reject(new Error('Failed to initialize JurisChain ASM module: ' + (err && err.message ? err.message : String(err))));
      });
    } catch (err) {
      if (!settled) {
        settled = true;
        cleanup();
        reject(new Error('Failed to start JurisChain ASM module: ' + (err && err.message ? err.message : String(err))));
      }
    }
  });
}

function solveJurischain(options) {
  var opts = options || {};
  var seed = opts.seed;
  var difficulty = opts.difficulty;
  var rawTimeout = opts.timeout;

  if (typeof seed !== 'string' || seed.length === 0) {
    return Promise.reject(new TypeError('seed must be a non-empty string'));
  }

  if (
    typeof difficulty === 'boolean' ||
    difficulty === null ||
    difficulty === '' ||
    Array.isArray(difficulty)
  ) {
    return Promise.reject(new RangeError('difficulty must be an integer between 1 and 255'));
  }

  var diff = Number(difficulty);
  if (!isFinite(diff) || Math.floor(diff) !== diff || diff < 1 || diff > 255) {
    return Promise.reject(new RangeError('difficulty must be an integer between 1 and 255'));
  }

  var timeout = 0;
  if (rawTimeout !== undefined) {
    if (
      typeof rawTimeout === 'boolean' ||
      rawTimeout === null ||
      rawTimeout === '' ||
      Array.isArray(rawTimeout)
    ) {
      return Promise.reject(new RangeError('timeout must be a non-negative finite number'));
    }
    timeout = Number(rawTimeout);
    if (!isFinite(timeout) || timeout < 0) {
      return Promise.reject(new RangeError('timeout must be a non-negative finite number'));
    }
  }

  _solveQueue = _solveQueue.then(function () {
    return _solve(seed, diff, timeout);
  }, function () {
    return _solve(seed, diff, timeout);
  });

  return _solveQueue;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = { solveJurischain: solveJurischain };
} else if (typeof define === 'function' && define.amd) {
  define(function () { return { solveJurischain: solveJurischain }; });
}
root.solveJurischain = solveJurischain;
