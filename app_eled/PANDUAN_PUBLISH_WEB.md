# Panduan memublikasikan Pengmas Jambi sebagai website

## Perubahan yang sudah disiapkan
- Folder `web/` sudah tersedia pada proyek ini.
- Pemanggilan `Workmanager` kini dilewati pada Web karena tugas background periodik Android tidak berjalan di browser.
- Metadata judul dan deskripsi halaman web diperbarui.
- `firebase.json` ditambahkan dengan folder publik `build/web` dan rewrite untuk Flutter SPA.

## 1. Prasyarat
- Instal Flutter SDK yang kompatibel dengan `pubspec.yaml` proyek ini.
- Instal Google Chrome.
- Instal Node.js LTS dari https://nodejs.org/
- Siapkan akun Google untuk Firebase Console.

## 2. Uji versi web secara lokal
Buka terminal di folder `app_eled` (folder yang berisi `pubspec.yaml`), lalu jalankan:

```powershell
flutter doctor
flutter pub get
flutter devices
flutter run -d chrome
```

Uji login, registrasi, dashboard, data lahan, lokasi, data cuaca BMKG, dan fitur lain yang digunakan.

## 3. Build website

```powershell
flutter build web --release
```

Hasil build dibuat di `build/web`. Jangan mengedit file hasil build secara manual.

## 4. Siapkan Firebase Hosting
1. Buka https://console.firebase.google.com/ dan buat proyek Firebase.
2. Instal Firebase CLI:

```powershell
npm install -g firebase-tools
firebase login
```

3. Dari folder `app_eled`, jalankan:

```powershell
firebase init hosting
```

Pilih proyek Firebase yang dibuat tadi. Jika ditanya direktori publik, masukkan `build/web`. Pilih `Yes` untuk single-page app rewrite. Jika ditanya apakah akan menimpa `index.html`, pilih `No`. File `firebase.json` pada proyek ini telah disiapkan, sehingga periksa agar konfigurasi tetap menggunakan `build/web`.

4. Deploy:

```powershell
flutter build web --release
firebase deploy --only hosting
```

Firebase akan menampilkan URL publik seperti `https://nama-proyek.web.app`. Bagikan URL yang benar-benar muncul pada terminal.

## Catatan kompatibilitas yang penting
- `Workmanager` dan notifikasi lokal yang berjalan di Android tidak memiliki perilaku identik di browser. Pada versi Web, penjadwalan background Android dilewati.
- Browser hanya memberikan akses GPS setelah izin pengguna dan umumnya memerlukan HTTPS.
- Jika login/registrasi atau API gagal di browser tetapi berhasil di Android, periksa tab Console/Network pada DevTools. Salah satu penyebab umum adalah kebijakan CORS di server API. CORS harus diperbaiki di server API; jangan mencoba mengatasi dengan menonaktifkan keamanan browser.
- Endpoint API yang digunakan aplikasi saat ini ada di `lib/services/api_service.dart`. Pastikan backend masih aktif dan mengizinkan origin website. Jangan menyimpan password atau secret di kode frontend.
- Saya belum dapat menjalankan `flutter build web` pada lingkungan ini karena Flutter SDK tidak tersedia. Jalankan langkah pengujian di atas pada komputer yang memiliki Flutter, lalu tangani error build jika ada.
