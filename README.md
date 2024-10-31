<img src="https://static.wikia.nocookie.net/arnelify/images/c/c8/Arnelify-logo-2024.png/revision/latest?cb=20240701012515" style="width:336px;" alt="Arnelify Logo" />

![Arnelify](https://img.shields.io/badge/Arnelify%20POD%20for%20C++-0.5.2-blue) ![C++](https://img.shields.io/badge/C++-23.0-blue)

## 🚀 About
**Arnelify® POD for C++** - is a framework for creating scalable microservices applications.

✅ The place for setting up routes and subscriptions:<br/>
```
./src/routes.hpp
```

## 📦 Installation
Install to the folder that needs to be created:
```
git clone https://github.com/arnelify/arnelify-pod-cpp.git
```

Create .env from .env.local:
```
cd arnelify-pod-cpp
cp .env.local .env
```

Run Docker:
```
docker compose up -d
```

## 🎉 Usage
Install dependencies:
```
make setup
```
Compile pod:
```
make build
```
Run development:
```
make watch
```
Migrate:
```
make migrate
```
Seed:
```
make seed
```
## ⭐ Release Notes
Version 0.5.2 - First Raw Release

We are excited to introduce the first version of Arnelify POD framework. Please note that this version is raw and still in active development. It includes only basic features, and the framework has not yet undergone thorough testing.

Key Features:

* Basic functionality
* Easy integration

Please use this version with caution, as it may contain bugs and unfinished features. We are actively working on improving and expanding the framework's capabilities, and we welcome your feedback and suggestions.