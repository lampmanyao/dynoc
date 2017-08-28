# DYNOC

Dynoc is a minimalistic C client library for the [dynomite](https://github.com/Netflix/dynomite).

# Features
- Connect pool，token aware.
- Read/Write retry when read/write from/to the local rack.
- Health check.

# Build
```
git clone https://github.com/lampmanyao/dynoc.git
cd dynoc/hiredis
make
cd src
make or make debug
```

# Supported Redis Commands
- SET
- GET
- DEL
- HSET
- HGET

# TODOs
- The remain features of Dyno (Official Client for dynomite)

