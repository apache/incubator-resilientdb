# Installing Dependencies for Copied Repositories

After copying ResLens, ResLens-Middleware, and nexus into graphq-llm, you need to install dependencies for each.

## ✅ ResLens - COMPLETE

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm/ResLens
npm install
```

**Status:** ✅ Installed 1682 packages
**Note:** 39 vulnerabilities detected (can be addressed later with `npm audit fix`)

---

## 📦 ResLens-Middleware - TODO

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm/ResLens-Middleware/middleware
npm install
```

---

## 📦 Nexus - TODO

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm/nexus
npm install
```

---

## 🔧 Optional: Fix Vulnerabilities (Later)

After all dependencies are installed, you can optionally address vulnerabilities:

```bash
# In each directory:
npm audit fix

# Or for more aggressive fixes (may break things):
npm audit fix --force
```

**Note:** It's safe to proceed with setup even with vulnerabilities. Fix them later if needed.

---

## ✅ Quick Install All

Run this to install dependencies in all three directories:

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm

# ResLens (already done)
cd ResLens && npm install && cd ..

# ResLens-Middleware
cd ResLens-Middleware/middleware && npm install && cd ../..

# Nexus
cd nexus && npm install && cd ..
```

---

## 🚀 After Installation

Once all dependencies are installed, you can run:

```bash
./fix-reslens.sh
```

This will start all services using the local copies.

