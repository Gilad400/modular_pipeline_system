# Modular Pipeline System  

## 📌 Overview  
This project is a **multithreaded, plugin-based string analyzer pipeline** written in C.  
It was developed as my **final project** in the Operating Systems course at Reichman University.  

The system simulates a real-world data-processing pipeline where each component (plugin) performs a unique transformation on input strings.  
Plugins are loaded dynamically at runtime as shared objects (`.so`), each running in its own thread and communicating via thread-safe queues.  

The project highlights key operating systems concepts such as:  
- Multithreading and synchronization  
- Producer-consumer model with bounded queues  
- Dynamic loading (`dlopen`, `dlsym`)  
- Modular software design  

---

## ⚙️ Features  
- **Dynamic plugin system** – load and arrange plugins at runtime  
- **Thread-safe communication** – bounded producer-consumer queues  
- **Graceful shutdown** – system terminates cleanly on `<END>` input  
- **Multiple plugins supported**, including:  
  - `logger` – logs all strings  
  - `uppercaser` – converts text to uppercase  
  - `rotator` – rotates characters  
  - `flipper` – reverses strings  
  - `expander` – adds spaces between characters  
  - `typewriter` – prints text with delays  

## 📂 Project Structure
```bash
├── main.c
├── build.sh
├── test.sh
├── plugins/
│   ├── plugin_common.c
│   ├── plugin_common.h
│   ├── plugin_sdk.h
│   ├── logger.c
│   ├── uppercaser.c
│   ├── rotator.c
│   ├── flipper.c
│   ├── expander.c
│   ├── typewriter.c
│   └── sync/
│       ├── monitor.c
│       ├── monitor.h
│       ├── consumer_producer.c
│       └── consumer_producer.h

---

## 🚀 Example Usage  
```bash
# Build the project
./build.sh

# Run a pipeline: uppercaser → rotator → logger
echo "hello" | ./output/analyzer 10 uppercaser rotator logger
echo "<END>" | ./output/analyzer 10 uppercaser rotator logger
