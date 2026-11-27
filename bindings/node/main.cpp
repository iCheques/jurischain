#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <nan.h>

#include "jurischain.h"

#define JURISCHAIN_MAX_SEED_LEN 1024

static inline int hex_char_to_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

class Jurischain : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init) {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::FunctionTemplate> ctor =
        Nan::New<v8::FunctionTemplate>(Jurischain::New);

    ctor->SetClassName(Nan::New("Jurischain").ToLocalChecked());
    ctor->InstanceTemplate()->SetInternalFieldCount(1);

    v8::Local<v8::Signature> sig = v8::Signature::New(isolate, ctor);
    v8::Local<v8::ObjectTemplate> proto = ctor->PrototypeTemplate();

    proto->Set(Nan::New("challengeResponse").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(Define, v8::Local<v8::Value>(), sig));
    proto->Set(Nan::New("readChallenge").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(Retrieve, v8::Local<v8::Value>(), sig));
    proto->Set(Nan::New("solveStep").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(Solve, v8::Local<v8::Value>(), sig));
    proto->Set(Nan::New("verify").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(Verify, v8::Local<v8::Value>(), sig));

    Nan::Set(target, Nan::New("Jurischain").ToLocalChecked(),
             Nan::GetFunction(ctor).ToLocalChecked());
  }

private:
  jurischain_ctx_t context;

  static Jurischain *UnwrapChecked(const Nan::FunctionCallbackInfo<v8::Value> &info) {
    if (info.This()->InternalFieldCount() < 1) {
      Nan::ThrowTypeError("Illegal invocation");
      return nullptr;
    }
    Jurischain *obj = Nan::ObjectWrap::Unwrap<Jurischain>(info.This());
    if (obj == nullptr) {
      Nan::ThrowTypeError("Illegal invocation");
      return nullptr;
    }
    return obj;
  }

  explicit Jurischain(uint8_t d, const void *seed, size_t inlen) {
    std::memset(&context, 0, sizeof(context));
    jurischain_gen(&context, d, seed, inlen);
  }

  ~Jurischain() {
    std::memset(&context, 0, sizeof(context));
  }

  static NAN_METHOD(New) {
    if (!info.IsConstructCall()) {
      return Nan::ThrowError("Jurischain must be called with the new keyword");
    }

    if (info.Length() < 2) {
      return Nan::ThrowTypeError("expected 2 arguments: difficulty (number) and seed (string)");
    }

    if (!info[0]->IsNumber()) {
      return Nan::ThrowTypeError("difficulty must be a number");
    }

    double rawDifficulty = Nan::To<double>(info[0]).FromMaybe(0.0);

    if (!std::isfinite(rawDifficulty) || std::floor(rawDifficulty) != rawDifficulty) {
      return Nan::ThrowTypeError("difficulty must be an integer");
    }

    if (rawDifficulty < 1.0 || rawDifficulty > 255.0) {
      return Nan::ThrowRangeError("difficulty must be between 1 and 255");
    }

    if (!info[1]->IsString()) {
      return Nan::ThrowTypeError("seed must be a string");
    }

    v8::Local<v8::String> seedStr = info[1].As<v8::String>();
    if (seedStr->Length() == 0) {
      return Nan::ThrowTypeError("seed must be a non-empty string");
    }

    if (seedStr->Length() > JURISCHAIN_MAX_SEED_LEN) {
      return Nan::ThrowRangeError("seed must not exceed 1024 bytes");
    }

    Nan::Utf8String seed(seedStr);

    if (seed.length() <= 0 || *seed == nullptr) {
      return Nan::ThrowTypeError("seed must be a non-empty string");
    }

    if (seed.length() > JURISCHAIN_MAX_SEED_LEN) {
      return Nan::ThrowRangeError("seed must not exceed 1024 bytes");
    }

    uint8_t difficulty = static_cast<uint8_t>(rawDifficulty);
    Jurischain *obj = new Jurischain(difficulty,
        static_cast<const void *>(*seed), static_cast<size_t>(seed.length()));
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(Verify) {
    Jurischain *obj = UnwrapChecked(info);
    if (!obj) return;
    info.GetReturnValue().Set(
        Nan::New<v8::Boolean>(jurischain_verify(&obj->context) != 0));
  }

  static NAN_METHOD(Solve) {
    Jurischain *obj = UnwrapChecked(info);
    if (!obj) return;
    info.GetReturnValue().Set(
        Nan::New<v8::Boolean>(jurischain_try(&obj->context) != 0));
  }

  static NAN_METHOD(Define) {
    Jurischain *obj = UnwrapChecked(info);
    if (!obj) return;

    if (info.Length() < 1) {
      return Nan::ThrowTypeError("expected 1 argument: hex response string");
    }

    if (!info[0]->IsString()) {
      return Nan::ThrowTypeError("response must be a string");
    }

    const int expectedLen = HASH_LEN * 2;
    v8::Local<v8::String> responseStr = info[0].As<v8::String>();
    if (responseStr->Length() != expectedLen) {
      return Nan::ThrowRangeError(
          "response must be exactly 64 hex characters");
    }

    Nan::Utf8String response(responseStr);

    if (response.length() != expectedLen) {
      return Nan::ThrowRangeError(
          "response must be exactly 64 hex characters");
    }

    const char *hex = *response;
    if (hex == nullptr) {
      return Nan::ThrowTypeError("response string is invalid");
    }

    uint8_t parsed[HASH_LEN];
    for (size_t i = 0; i < HASH_LEN; i++) {
      int hi = hex_char_to_nibble(hex[i * 2]);
      int lo = hex_char_to_nibble(hex[i * 2 + 1]);
      if (hi < 0 || lo < 0) {
        char errmsg[64];
        std::snprintf(errmsg, sizeof(errmsg),
            "response contains invalid hex at position %zu", i * 2);
        return Nan::ThrowError(errmsg);
      }
      parsed[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    std::memcpy(obj->context.seed, parsed, HASH_LEN);
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(true));
  }

  static NAN_METHOD(Retrieve) {
    Jurischain *obj = UnwrapChecked(info);
    if (!obj) return;

    char str[(HASH_LEN * 2) + 1] = { 0 };
    for (int i = 0; i < HASH_LEN; i++) {
      std::snprintf(str + (i * 2), 3, "%02hhX", obj->context.seed[i]);
    }

    info.GetReturnValue().Set(Nan::New<v8::String>(str, HASH_LEN * 2).ToLocalChecked());
  }
};

NAN_MODULE_WORKER_ENABLED(Jurischain, Jurischain::Init)
