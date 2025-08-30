# Burn2Slot

**Burn2Slot** is a Nintendo DS homebrew utility that allows you to reflash Game Boy Advance (GBA) bootleg cartridges through Slot-2. This is a convenient solution for rewriting compatible GBA carts directly from a DS or DS Lite, without the need for external hardware or a PC.

---

## 📦 Features

- Flash `.gba` ROMs onto writable GBA bootleg cartridges
- Runs from Slot-1 flashcarts (e.g. R4, Acekard)
- Works on Nintendo DS and DS Lite
- Fast and easy ROM programming via on-screen menu

---

## 🧰 Requirements

- Nintendo DS or DS Lite  
- Slot-1 flashcart (e.g. R4, Acekard, etc.)  
- Writable GBA bootleg cartridge in Slot-2  
- GBA ROMs (must fit within cart capacity, e.g., 8 MB or 16 MB)

---

## 🚀 How to Use

1. Place `Burn2Slot.nds` on your Slot-1 flashcart.
2. Add your desired `.gba` ROM file(s) to the flashcart.
3. Insert a compatible bootleg GBA cartridge into Slot-2.
4. Launch `Burn2Slot.nds` from the DS menu.
5. Follow the on-screen instructions to select and flash the ROM.

> ⚠️ This process will overwrite the contents of your bootleg GBA cart. Proceed with caution.

---

## 🛠️ Building from Source

```bash
git clone https://github.com/vrodin/Burn2Slot.git
cd Burn2Slot
make
```

---

## 🛠️ Dependencies

- [devkitPro](https://devkitpro.org/)
- devkitARM
- libnds

Make sure your environment variables (`DEVKITPRO` and `DEVKITARM`) are correctly set.

---

## 🧪 Compatibility

- Bootleg GBA cartridges with NOR flash
- Recommended sizes: **8 MB**, **16 MB**, possibly **32 MB**
- Does *not* support EEPROM or SRAM-based flash carts

Not all bootleg cartridges are supported. Compatibility may vary depending on manufacturer and flash chip.

---

## 🙌 Credits

Developed by [vrodin](https://github.com/vrodin)

Inspired by other DS/GBA homebrew flasher projects.

---

## Research and References

- [Console Technical Info](http://problemkaputt.de/gbatek.htm)
- [Common Flash Memory Interface](https://en.wikipedia.org/wiki/Common_Flash_Memory_Interface)
- [Flash Memory Interface](https://www.fujitsu.com/downloads/MICRO/fmal/e-ds/e520904.pdf)
- [cartreader by sanni](https://github.com/sanni/cartreader)
- [nds-gbabf by nflsilva](https://github.com/nflsilva/nds-gbabf)
- [gbabf by Fexean](https://gitlab.com/Fexean/gbabf)
- [GBxCart-RW by insidegadgets](https://github.com/insidegadgets/GBxCart-RW)
