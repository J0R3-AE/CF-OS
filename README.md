# 🚀 CF‑OS  

<sub>*“Not rivaling Windows… yet.”*</sub>

CF‑OS is a tiny, chaotic, experimental, monolithic hobby operating system built for **x86**, written in **C**, booted by **GRUB**, and powered by equal parts curiosity and cocaine.

---

## ? What CF‑OS Is

A small OS kernel featuring:

- **Physical Memory Manager (PMM)**  
  Bitmap‑based, frame‑aligned, and surprisingly well‑behaved.

- **Kernel Heap (malloc/free)**  
  Linked‑list free blocks, coalescing, and all the usual foot‑guns.

- **VGA Text Mode (deprecated)**  
  It lived. It died. It was replaced by something better.

- **TTY Subsystem**  
  A real terminal abstraction with framebuffer backend support.  
  (VGA text mode is now the weird uncle we don’t talk about.)

- **Paging + VMM**  
  Page directories, page tables, identity mapping, CR3 switching — the whole x86 circus.

- **GRUB Multiboot Bootloader**  
  Because writing your own bootloader is how you lose your sanity.

- 🗑️ **Userland**  
  Removed for now. It will return when the kernel stops screaming.

---

## 🔮 Roadmap  

<sub>*a.k.a. “Things Future JJC Will Regret”*</sub>

### 🧩 Memory & Kernel

- Full x86_64 support  
- Higher‑half kernel  
- Proper slab allocator  
- Copy‑on‑write  
- Kernel ASLR

### 🪟 Graphics & UI

- Framebuffer driver improvements  
- ANSI colors, cursor, scrolling  
- Double buffering  
- GUI experiments  
- Window manager (eventually)

### 🧍 Userland

- ELF loader  
- Syscalls  
- Process scheduler  
- Shell  
- Real filesystem (EXT2 or bust)

### 🔌 Drivers

- PS/2 keyboard  
- PIT/HPET timers  
- RTC  
- Eventually: PCI, AHCI, maybe even USB if you’re brave

---

## 🏗️ Build Status
>
> ⚠️ *If it boots, that counts as a success.*

---

## 🧬 Tech Stack

| Component | Status |
| **Architecture** | x86 (x86_64 planned) |
| **Language** | C | NASM |
| **Bootloader** | GRUB Multiboot |
| **Memory Model** | Paging + PMM + Kernel Heap |
| **Console** | TTY (Framebuffer backend) |

---

## 📸 Screenshots

- *(None yet srry not srry.)*

---

## 📜 License

MIT, GPL, BSD, proprietary, "please don’t sue me" IT

---

## 🤝 Contributions

PRs welcome.  
Bug reports welcome.  
Complaints about the code quality… also welcome, but rude.

---

> **CF‑OS will rival Windows.**

But not today.  
Today, we celebrate the boot log.
