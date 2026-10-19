# Dokumentasi Perangkat: tester-water-sukawesi

Device ID: `cec53d74-b86f-447b-8baf-00e61a21515c`

> Simpan dokumen ini di tempat aman. Kredensial di bawah hanya ditampilkan satu kali dan tidak bisa dilihat ulang lewat dashboard.

## Urutan Sensor (posisi kolom CSV)

Urutan ini ditentukan sesuai sensor yang terdaftar di stasiun ini saat dibuat/diedit. Payload CSV WAJIB mengikuti urutan posisi ini persis.

| Posisi (kolom CSV) | Key | Label |
|---|---|---|
| 0 | `gps_lat` | Latitude GPS |
| 1 | `gps_lon` | Longitude GPS |
| 2 | `turbidity` | Turbidity |
| 3 | `tinggi_muka_air` | Tinggi Muka Air |
| 4 | `battery` | Battery |
| 5 | `profile` | Profile |

## Pengiriman Data via MQTT

- **Broker (MQTT over WebSocket)**: `wss://monitoring.pantaudas.cloud/mqtt`
- **Subprotocol WebSocket**: `mqtt`
- Port TCP 1883 TIDAK dibuka ke internet — gunakan URL WebSocket di atas.
- **Username**: `device_tester_water_sukawesi`
- **Password**: `7d4a91651108b30afb438f24f3b8b3b15abfe20d6a35c0c3`
- **QoS**: `2`

Kredensial ini divalidasi langsung oleh broker (bukan oleh server aplikasi) — device dengan username/password salah akan ditolak koneksi. Perangkat hanya diberi akses ke topic miliknya sendiri (di bawah), bukan ke topic perangkat lain.

### Topic

| Topic | Arah | Keterangan |
|---|---|---|
| `devices/tester-water-sukawesi/data` | publish (device→server) | data sensor, format sama seperti REST (CSV atau JSON satu-sensor) |
| `devices/tester-water-sukawesi/cmd` | subscribe (server→device) | instruksi dari admin (set interval, kalibrasi, reset, sync time, dll) |
| `devices/tester-water-sukawesi/cmd/ack` | publish (device→server) | WAJIB dibalas device setelah instruksi dieksekusi |
| `devices/tester-water-sukawesi/ota` | subscribe (server→device) | perintah OTA update firmware |
| `devices/tester-water-sukawesi/ota/status` | publish (device→server) | status hasil OTA (in-progress tidak wajib, final wajib) |
| `devices/tester-water-sukawesi/ping` | subscribe (server→device) | ping dari admin/teknisi untuk cek konektivitas |
| `devices/tester-water-sukawesi/pong` | publish (device→server) | balasan ping, secepat mungkin |

### 1. Data — publish ke `/data`

Sama seperti REST: CSV (direkomendasikan) atau JSON satu-sensor.

```
24.6;81.2;6.8;3;1013.2;0.4
```

```json
{ "sensorKey": "gps_lat", "value": 27.4 }
```

### 2. Instruksi (Command) — subscribe `/cmd`, wajib balas `/cmd/ack`

Server publish payload berikut saat admin mengirim instruksi (mis. ubah interval kirim, kalibrasi ulang, reset, atau sinkronisasi waktu):

```json
{ "commandId": "uuid-instruksi", "sensorId": null, "payload": "{\"type\":\"set_interval\",\"intervalSeconds\":300,\"startDate\":null,\"endDate\":null}" }
```

Device **WAJIB** membalas ke `/cmd/ack` (idealnya dalam <10 detik — setelah itu server menandai instruksi gagal/timeout di dashboard, meski instruksi tetap tersimpan untuk dicoba lagi):

```json
{ "commandId": "uuid-instruksi", "success": true, "message": "Interval diterapkan" }
```

Beberapa `payload.type` yang dikenal dashboard: `set_interval` (lihat contoh di atas), `calibrate`, `reset`, `sync_time`. Device boleh menambahkan tipe instruksi sendiri sesuai kebutuhan — server hanya meneruskan payload apa adanya, tidak memvalidasi isinya.

### 3. OTA (Over-The-Air firmware update) — subscribe `/ota`, wajib lapor `/ota/status`

Alur OTA:

1. Admin memilih firmware di dashboard → server publish ke `/ota`:
   ```json
   { "jobId": "uuid-job", "url": "https://...presigned-url-1-jam...", "version": "1.4.0" }
   ```
2. Device mengunduh firmware dari `url` (presigned URL berlaku 1 jam), lalu memasangnya.
3. Setelah selesai (berhasil ATAU gagal), device **WAJIB** publish ke `/ota/status`:
   ```json
   { "success": true, "message": "Firmware 1.4.0 terpasang" }
   ```
4. Jika tidak ada laporan status dalam 5 menit, server menandai job tersebut `failed` otomatis (`"No response from device"`) — device tetap boleh lapor belakangan tapi tidak akan mengubah status yang sudah timeout.

### 4. Ping — subscribe `/ping`, wajib balas `/pong`

```json
{ "pingId": "uuid-ping", "ts": "2026-09-30T06:00:00.000Z" }
```

Device balas secepat mungkin ke `/pong` (boleh payload kosong/apa saja — server hanya perlu tanda terima untuk menghitung latensi):

```json
{ "pong": true, "ts": "2026-09-30T06:00:00.150Z" }
```

## Analisis Latensi Komunikasi

Setiap jenis komunikasi mencatat durasinya sendiri untuk dianalisis lewat dashboard/API — tabel di bawah menjelaskan apa yang diukur dan batasannya:

| Komunikasi | Field tersimpan | Yang benar-benar diukur |
|---|---|---|
| Data masuk (station→server, REST/MQTT) | `IngestionLog.processingMs` (per pembacaan sensor) | Waktu pemrosesan DI SISI SERVER saja (dari pesan diterima sampai nilai selesai diproses, termasuk waktu antre di queue) — **bukan** waktu transit jaringan station→server, karena payload data tidak membawa jam perangkat yang tersinkron. |
| Instruksi (server→station→server) | `CommandLog.sentAt`, `ackAt`, `latencyMs` | Waktu pulang-pergi penuh: sejak server publish ke `/cmd` sampai `/cmd/ack` diterima kembali. Ini yang paling representatif sebagai "latensi MQTT" karena murni diukur di jam server pada kedua ujungnya. |
| OTA (server→station→server) | `OtaUpdateJob.startedAt`, `completedAt`, `durationMs` | Total waktu dari perintah OTA dikirim sampai device melaporkan hasil akhir — termasuk waktu unduh+pasang firmware, bukan cuma transit jaringan. |
| Ping (server→station→server) | `PingTest.requestedAt`, `respondedAt`, `latencyMs` | Waktu pulang-pergi murni `/ping`→`/pong`, indikator paling bersih untuk kondisi jaringan device saat ini (tanpa waktu pemrosesan tambahan seperti OTA). |

Untuk mengukur latensi transit jaringan station→server yang sesungguhnya (bukan sekadar waktu proses server), perangkat perlu menyertakan jam perangkatnya sendiri di setiap pesan data — ini belum menjadi bagian dari format payload saat ini dan memerlukan perubahan protokol wire format bila dibutuhkan di kemudian hari.
