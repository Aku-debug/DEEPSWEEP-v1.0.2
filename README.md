<h1 align="center" style="color:#4CAF50;">🧹 Windows Uygulama Temizleyici</h1>
<p align="center"><b><i>Bilgisayarınızdaki kurulu uygulamaları tespit eder, kaldırır ve arkada kalan kalıntıları temizler!</i></b></p>

---

## ✨ Uygulama Hakkında

Bu araç, Windows işletim sisteminde kurulu olan tüm yazılımları detaylı şekilde listeler ve kullanıcıya kaldırma seçeneği sunar.  
Ayrıca kaldırılan programların geride bıraktığı dosya kalıntılarını da temizleyerek sistemi sadeleştirir.

---

## ⚙️ Çalışma Prensibi

🔍 **1. Tarama**  
- Uygulama; 64-bit, 32-bit ve kullanıcıya özel kurulumları WMI (Windows Management Instrumentation) üzerinden tespit eder.  
- Her bir yazılım için aşağıdaki bilgileri çeker:
  - 📦 Program Adı  
  - 🏢 Yayıncı  
  - 📅 Kurulum Tarihi  
  - 📁 Kurulum Yolu  
  - 📐 Boyutu  

🗑️ **2. Kaldırma**  
- Seçilen uygulama, sessiz mod destekliyorsa arka planda kaldırılır.  
- Aksi takdirde normal kaldırıcı penceresi kullanıcıya gösterilir.  

🧽 **3. Temizlik**  
- Aşağıdaki klasörlerde kalan dosya ve klasörler taranarak silinir:
  - `C:\Program Files\`  
  - `C:\Program Files (x86)\`  
  - `%AppData%`  
  - `%LocalAppData%`  

🖥️ **4. Terminal Çıktısı**  
- Her adım renkli terminal çıktıları ile kullanıcıya bildirilir:  
  - 🟢 Başarılı işlemler (yeşil)  
  - 🟡 Uyarılar (sarı)  
  - 🔴 Hatalar (kırmızı)  

---

## ✅ Neden Kullanmalı?

✔️ Gereksiz yazılımları sistemden tamamen kaldırır  
✔️ Elle silinmesi zor olan kalıntıları otomatik temizler  
✔️ Renkli ve anlaşılır terminal arayüzü  
✔️ Hafif, bağımsız, kurulum gerektirmez

---

<p align="center"><b>Sisteminizi temiz tutun, gereksiz yüklerden kurtulun! 🧼</b></p>
