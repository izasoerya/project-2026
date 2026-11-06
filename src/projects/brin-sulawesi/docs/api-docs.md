# Dokumentasi Perangkat: tester-water-sukawesi

|                  |                                                               |
| ---------------- | ------------------------------------------------------------- |
| Device ID        | `cec53d74-b86f-447b-8baf-00e61a21515c`                        |
| Jalur data aktif | **MQTT**                                                      |
| Base URL API     | `https://monitoring.pantaudas.cloud/api`                      |
| Broker MQTT      | `wss://monitoring.pantaudas.cloud/mqtt` (MQTT over WebSocket) |
| Dokumen dibuat   | 2026-11-06                                                    |

> Kredensial tidak disertakan: yang tersimpan di server hanya hash-nya. Tanda `<...>` di bawah adalah placeholder — pakai kredensial yang Anda simpan saat digenerate, atau generate ulang jalur data stasiun ini (kredensial lama langsung tidak berlaku).

## 1. Ringkasan jalur komunikasi

| Jalur                                                    | Arah            | Fungsi                       | Diproses sebagai                                                                   |
| -------------------------------------------------------- | --------------- | ---------------------------- | ---------------------------------------------------------------------------------- |
| REST `POST /ingest/cec53d74-b86f-447b-8baf-00e61a21515c` | device → server | kirim data sensor            | rata-rata per menit, alert, riwayat                                                |
| MQTT `devices/tester-water-sukawesi/data`                | device → server | kirim data sensor            | sama dengan REST                                                                   |
| MQTT `devices/tester-water-sukawesi/stream`              | device → server | stream data mentah real-time | **hanya** tampil live di dashboard + log sampel (tidak disimpan sebagai pembacaan) |
| MQTT `devices/tester-water-sukawesi/cmd` · `/cmd/ack`    | server ⇄ device | instruksi ke perangkat       | antrean per stasiun, ack maks. 2 jam; retained (default) atau tanpa retain         |
| MQTT `devices/tester-water-sukawesi/ota` · `/ota/status` | server ⇄ device | update firmware              | job OTA, status maks. 2 jam                                                        |
| MQTT `devices/tester-water-sukawesi/ping` · `/pong`      | server ⇄ device | uji konektivitas             | latensi pulang-pergi                                                               |

Satu stasiun memakai **satu** jalur data (REST _atau_ MQTT). REST murni satu arah (device → server); instruksi, OTA, ping dan stream hanya tersedia lewat MQTT.

## 2. Urutan sensor (kolom CSV)

Urutan ini mengikuti sensor **aktif** yang terdaftar di stasiun ini saat dibuat/diedit. Payload CSV WAJIB mengikuti urutan kolom ini persis (kolom 1 = sensor pertama, dst). Mengubah urutan/aktivasi sensor di dashboard mengubah pemetaan kolom — unduh ulang dokumen ini setelahnya.

| Kolom CSV | Key                  | Label              |
| --------- | -------------------- | ------------------ |
| 1         | `gps_lat`            | Latitude GPS       |
| 2         | `gps_lon`            | Longitude GPS      |
| 3         | `turbidity`          | Turbidity          |
| 4         | `tinggi_muka_air`    | Tinggi Muka Air    |
| 5         | `battery`            | Battery            |
| 6         | `profile`            | Profile            |
| 7         | `rssi`               | RSSI               |
| 8         | `free_heap`          | free heap          |
| 9         | `min_free_heap`      | min free heap      |
| 10        | `largest_block_heap` | largest block heap |

## 3. Format payload data

Dipakai sama persis oleh REST, MQTT `/data` dan MQTT `/stream`. Format dideteksi otomatis dari isi pesan.

### CSV (direkomendasikan)

Satu baris teks, nilai dipisah koma (`,`), urutan sesuai tabel kolom di atas. Pemisah titik koma (`;`) juga diterima. **Desimal memakai titik** (`-7.836775`); desimal berkoma (`6,8`) hanya diterima pada format titik koma (`6,8;7,1`).

```
24.6,81.2,6.8,3,1013.2,0.4,24.6,81.2,6.8,3
```

### JSON satu-sensor (kompatibilitas lama)

Pesan yang diawali `{` dibaca sebagai satu pembacaan satu sensor:

```json
{ "sensorKey": "gps_lat", "value": 27.4 }
```

### Aturan

- Kolom CSV lebih **sedikit** dari jumlah sensor aktif: sensor yang tidak kebagian kolom dilewati pada siklus itu (pesan tetap diterima).
- Kolom CSV lebih **banyak**: kolom ekstra diabaikan dan pesan tetap diterima; server mencatat peringatan karena biasanya ini tanda urutan/jumlah sensor tidak cocok.
- Kolom kosong di akhir (`1,2,3,`) diabaikan; kolom kosong di tengah ditolak.
- Satu nilai yang bukan angka valid menolak **seluruh** pesan (`rejected`); alasannya tercatat di Log Ingestion, mis. `Field 2 (sensor "ph")`.
- Stasiun bergerak (`isMoving`): sensor `gps_lat` dan `gps_lon` di dalam CSV memperbarui posisi di peta — letakkan keduanya di kolom sesuai urutan sensor.
- Ukuran payload yang disimpan di log dibatasi 4 KB (lebih dari itu tetap diproses, hanya salinan log yang dipotong).

## 4. REST API

_Jalur REST **tidak diaktifkan** untuk stasiun ini (stasiun memakai MQTT). Endpoint `POST /ingest/<deviceId>` akan menjawab `401`._

## 5. MQTT

### Koneksi

- **Broker (MQTT over WebSocket)**: `wss://monitoring.pantaudas.cloud/mqtt`
- **Subprotocol WebSocket**: `mqtt`
- Port TCP 1883 tidak dibuka ke internet — gunakan URL WebSocket di atas.
- **Username**: `device_tester_water_sukawesi`
- **Password**: `<MQTT_PASSWORD_TERDAFTAR>`
- **QoS data**: `2`

Kredensial divalidasi langsung oleh broker. Perangkat hanya boleh memakai topic miliknya (tabel di bawah); publish ke topic lain atau topic yang tidak diizinkan **dibuang diam-diam** oleh broker tanpa error.

### Topic

| Topic                                      | Arah                      | Keterangan                                                                                        |
| ------------------------------------------ | ------------------------- | ------------------------------------------------------------------------------------------------- |
| `devices/tester-water-sukawesi/data`       | publish (device→server)   | data sensor (CSV/JSON), diproses penuh: rata-rata per menit, alert, riwayat                       |
| `devices/tester-water-sukawesi/stream`     | publish (device→server)   | **stream data mentah real-time** (format sama, lihat 5.2)                                         |
| `devices/tester-water-sukawesi/cmd`        | subscribe (server→device) | instruksi dari admin — **retained** secara default, admin dapat mengirim tanpa retain (5.3)       |
| `devices/tester-water-sukawesi/cmd/ack`    | publish (device→server)   | WAJIB dibalas setelah instruksi dieksekusi (batas 2 jam)                                          |
| `devices/tester-water-sukawesi/ota`        | subscribe (server→device) | perintah OTA firmware — **retained** (5.4)                                                        |
| `devices/tester-water-sukawesi/ota/status` | publish (device→server)   | progres OTA (`received`/`downloading`/`installing`) dan status akhir (wajib, batas 2 jam)         |
| `devices/tester-water-sukawesi/ping`       | subscribe (server→device) | ping dari admin/teknisi — biasanya tidak retained; teknisi dapat mengirim ping **retained** (5.5) |
| `devices/tester-water-sukawesi/pong`       | publish (device→server)   | balasan ping, secepat mungkin                                                                     |

### Pesan retained (`/cmd` dan `/ota`) dan perangkat yang tidur

Server mem-publish `/ota` — dan `/cmd` secara default — dengan flag **retain**: perangkat online menerimanya seketika, perangkat yang sedang tidur menerimanya begitu **subscribe (QoS 1)** setelah bangun — tidak perlu mengirim data lebih dulu. Aturan yang berlaku untuk keduanya:

- Subscribe `/cmd` dan `/ota` **setiap kali bangun**, lalu dengarkan beberapa detik.
- Retained dikirim ulang di **setiap subscribe** selama belum selesai ⇒ firmware **wajib dedupe** `commandId` / `jobId`: simpan id terakhir yang sudah dieksekusi; bila menerimanya lagi, jangan eksekusi ulang — cukup kirim ulang ack/status akhirnya.
- **Pesan kosong** (payload 0 byte) di `/cmd` atau `/ota` adalah server yang mengosongkan slot retained — bukan instruksi; abaikan.
- Setiap instruksi/OTA punya **batas 2 jam** sejak dikirim; lewat batas, ditandai `expired`/`failed` di dashboard dan pesan retained dihapus.
- Admin dapat mengirim `/cmd` **tanpa retain** (opsi _Retain di broker_ di panel Kirim Perintah, juga bisa diset pada payload tersimpan). Pesan tanpa retain **hanya diterima perangkat yang sedang online dan sudah subscribe saat dikirim**; broker tidak menyimpannya, jadi perangkat yang tidur/terputus melewatkannya dan tidak menerimanya saat subscribe berikutnya. Format pesan dan kewajiban `/cmd/ack` **sama persis** — firmware tidak perlu dibedakan. `/ota` selalu retained.

### 5.1 Data — publish ke `/data`

```
24.6,81.2,6.8,3,1013.2,0.4,24.6,81.2,6.8,3
```

```json
{ "sensorKey": "gps_lat", "value": 27.4 }
```

### 5.2 Stream data mentah — publish ke `/stream`

Untuk memantau data per detik secara langsung. Format payload **sama** dengan `/data`.

- Nilai dikalibrasi lalu **langsung ditampilkan live** di halaman _Log Ingestion → Stream Live_ (maks. ±2 pembaruan/detik per stasiun).
- **Tidak** dirata-rata, **tidak** disimpan ke riwayat pembacaan, dan **tidak** memicu alert/GPS — untuk itu tetap gunakan `/data`.
- Hanya satu sampel per ±5 detik per stasiun yang dicatat di Log Ingestion (lengkap dengan payload yang bisa dilihat); pesan yang rusak dicatat sebagai `rejected` dengan alasannya.
- Nilai stream juga tampil di halaman detail stasiun pada bagian _Konfigurasi Sensor & Rumus Kalibrasi_: per sensor ditampilkan **nilai raw dari alat** dan **nilai setelah rumus kalibrasi** (sensor tanpa rumus aktif: nilai terkalibrasi = raw). Rumus yang baru disimpan berlaku pada sampel stream berikutnya — cocok untuk kalibrasi di lapangan sambil stasiun mem-publish ke `/stream`. Kolom wajib mengikuti urutan pada bagian 2.
- QoS 0 cukup (best-effort); kehilangan satu sampel tidak berdampak.

```
24.6,81.2,6.8,3,1013.2,0.4,24.6,81.2,6.8,3
```

### 5.3 Instruksi (command) — subscribe `/cmd`, wajib balas `/cmd/ack`

Instruksi dari admin (ubah interval kirim, kalibrasi, reset, sinkron waktu, atau payload JSON bebas):

```json
{
  "commandId": "uuid-instruksi",
  "sensorKey": null,
  "payload": "{\"type\":\"set_interval\",\"intervalSeconds\":300,\"startDate\":null,\"endDate\":null}"
}
```

- `sensorKey` adalah **key sensor** target (sama dengan kolom pada bagian 2, mis. `"temp"`); `null` berarti perintah untuk seluruh perangkat. Instruksi dikirim berdasarkan key, bukan ID internal database.

Setelah dieksekusi, balas ke `/cmd/ack`:

```json
{
  "commandId": "uuid-instruksi",
  "success": true,
  "message": "Interval diterapkan"
}
```

- `success: false` ditandai `failed` di dashboard (pesan perangkat disimpan).
- Satu topik hanya menampung satu pesan retained ⇒ instruksi untuk satu stasiun dikirim **berurutan** (antrean FIFO): yang berikutnya baru dipublish setelah yang di depan di-ack/dibatalkan/kedaluwarsa. Perangkat yang masih terhubung menerimanya langsung setelah ack.
- Perintah tanpa retain tetap ikut antrean ini dan batas 2 jam: bila perangkat melewatkannya (sedang tidur), perintah itu menahan antrean sampai di-ack, dibatalkan admin, atau kedaluwarsa.
- `payload` adalah JSON object (dalam bentuk string); tipe yang dikenal dashboard: `set_interval`, `calibrate`, `reset`, `sync_time`. Tipe lain bebas ditambahkan firmware — server hanya meneruskannya.

### 5.4 OTA firmware — subscribe `/ota`, wajib lapor `/ota/status`

Server mengirim pesan OTA (retained) berisi alamat unduhan firmware:

```json
{
  "jobId": "uuid-job",
  "url": "https://...presigned-url...",
  "version": "1.4.0",
  "expiresAt": "2026-10-19T10:00:00.000Z"
}
```

| Field       | Isi                                                                                                         |
| ----------- | ----------------------------------------------------------------------------------------------------------- |
| `jobId`     | ID job OTA — sertakan di setiap laporan ke `/ota/status` dan jadikan kunci dedupe                           |
| `url`       | alamat unduhan (HTTPS, presigned, tanpa autentikasi tambahan); berlaku sampai ±30 menit setelah batas 2 jam |
| `version`   | versi firmware yang dikirim (label dari admin)                                                              |
| `expiresAt` | batas waktu job (2 jam sejak dikirim)                                                                       |

**Alur firmware**

1. Terima pesan `/ota`. Jika `jobId` sudah pernah selesai → jangan pasang lagi, cukup kirim ulang laporan akhir yang lama.
2. (Disarankan) kirim tanda terima ke `/ota/status` agar dashboard tahu perintah sudah diambil, mis. `{ "jobId": "uuid-job", "state": "received" }`. Progres opsional: `downloading` (boleh dengan `"progress": 40`) dan `installing`.
3. Unduh firmware dari `url`, verifikasi, lalu pasang.
4. Kirim **laporan akhir** ke `/ota/status` — berhasil ATAU gagal (unduhan gagal, checksum salah, flash gagal, dst.). Setelah laporan akhir diterima server mengosongkan pesan retained `/ota`.

**Format laporan `/ota/status` — server menerima pesan APA PUN sebagai ack**, tidak harus JSON dan tidak harus lengkap:

| Yang dikirim stasiun                                                                                                         | Dibaca sebagai                                                                                                                                                                                     |
| ---------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| JSON `{ "jobId": "…", "success": true, "message": "…" }` (**direkomendasikan**)                                              | laporan akhir berhasil; `success:false` = gagal (pesan disimpan di riwayat OTA)                                                                                                                    |
| JSON `{ "jobId": "…", "state": "received" \| "downloading" \| "installing", "progress": 40 }`                                | progres — job menjadi _in progress_, belum selesai                                                                                                                                                 |
| JSON dengan kata status: `"status"`/`"result"`/`"state"` = `ok`/`done`/`success`/`completed`… atau `failed`/`error`/`abort`… | berhasil / gagal                                                                                                                                                                                   |
| JSON dengan `"ok": true/false`, atau hanya `"error": "alasan"`                                                               | berhasil / gagal                                                                                                                                                                                   |
| JSON lain tanpa penanda apa pun (mis. `{ "message": "selesai" }`)                                                            | berhasil (ack biasa)                                                                                                                                                                               |
| **Teks biasa**: `OK`, `Firmware 1.4.0 terpasang`, `FAILED: flash error`                                                      | kata pertama menentukan: `ok/done/success/selesai…` atau teks lain = berhasil; `fail/error/gagal/abort/timeout…` = gagal; `received/downloading/installing` = progres. Teks disimpan sebagai pesan |
| JSON `true` / `false`                                                                                                        | berhasil / gagal                                                                                                                                                                                   |
| **Pesan kosong** (0 byte)                                                                                                    | ack biasa = berhasil                                                                                                                                                                               |

- `jobId` opsional: boleh berupa `jobId`, `job_id` atau `id` di JSON, atau UUID yang ditulis di dalam teks (`OK 3f2c…`). Tanpa `jobId`, laporan dianggap milik job aktif terakhir stasiun ini — **sertakan `jobId` bila stasiun bisa menerima lebih dari satu OTA**.
- Pesan pelengkap dibaca dari `message` (atau `msg`/`detail`/`reason`/`error`); maks. 500 karakter disimpan.
- Karena pesan apa pun dihitung sebagai laporan akhir (berhasil) kecuali berisi kata gagal/progres, **jangan** mengirim pesan lain ke `/ota/status` (log debug, heartbeat) — pakai topic lain atau tambahkan `"state"`.
- Laporan gagal → status _failed_ dan pesan retained dihapus; admin dapat mengirim ulang OTA.

**Status di dashboard**

| Status               | Arti                                                                    |
| -------------------- | ----------------------------------------------------------------------- |
| `pending`            | job dikirim ke broker, belum diambil stasiun                            |
| `in_progress`        | stasiun sudah mengambil (laporan `received`/`downloading`/`installing`) |
| `success` / `failed` | laporan akhir dari stasiun                                              |
| `expired`            | tidak diambil dalam 2 jam                                               |
| `cancelled`          | dibatalkan admin                                                        |

- Tidak ada tanda pengambilan dalam 2 jam ⇒ `expired`; sudah mulai tetapi tak ada laporan akhir ⇒ `failed`. Laporan akhir yang terlambat dengan `jobId` yang benar tetap diterima.
- Admin dapat membatalkan OTA yang belum diambil atau menggantinya dengan OTA baru (satu job aktif per stasiun).
- Pesan kosong di `/ota` dari server = slot retained dikosongkan, bukan perintah — abaikan.

### 5.5 Ping — subscribe `/ping`, wajib balas `/pong`

```json
{ "pingId": "uuid-ping", "ts": "2026-09-30T06:00:00.000Z" }
```

Balas secepat mungkin ke `/pong` (payload bebas — server hanya perlu tanda terima, timeout 5 detik):

```json
{ "pong": true, "ts": "2026-09-30T06:00:00.150Z" }
```

- Teknisi dapat mengirim ping **retained** (opsi _Retain_ di halaman Ping & Latensi) untuk stasiun yang sering tidur: pesan ping disimpan broker dan diterima saat stasiun **subscribe `/ping`** setelah bangun; balas ke `/pong` seperti biasa. Server menutup uji tersebut sebagai berhasil (latensi = permintaan → pong, termasuk waktu tidur) dan menghapus pesan retained; bila tak dijawab dalam **2 jam** pesan dihapus. Hanya satu ping retained per stasiun (yang baru menggantikan yang lama).
- Pesan kosong (0 byte) di `/ping` adalah penghapusan retained — abaikan, jangan dibalas.

## 6. Contoh klien

### mosquitto (uji cepat dari komputer)

```bash
# broker hanya tersedia lewat WebSocket: pakai klien yang mendukung ws(s), mis. MQTTX atau mqtt.js (di bawah)
```

### Node.js (mqtt.js)

```js
import mqtt from "mqtt";
const client = mqtt.connect("wss://monitoring.pantaudas.cloud/mqtt", {
  username: "device_tester_water_sukawesi",
  password: "<MQTT_PASSWORD_TERDAFTAR>",
  protocolVersion: 4,
});
const done = new Set(); // dedupe commandId / jobId
client.on("connect", () => {
  client.subscribe(
    [
      "devices/tester-water-sukawesi/cmd",
      "devices/tester-water-sukawesi/ota",
      "devices/tester-water-sukawesi/ping",
    ],
    { qos: 1 },
  ); // subscribe dulu...
  client.publish(
    "devices/tester-water-sukawesi/data",
    "24.6,81.2,6.8,3,1013.2,0.4,24.6,81.2,6.8,3",
    { qos: 2 },
  ); // ...baru kirim data
});
client.on("message", (topic, payload) => {
  if (payload.length === 0) return; // pesan kosong = retained dikosongkan
  const msg = JSON.parse(payload.toString());
  if (topic.endsWith("/cmd") && !done.has(msg.commandId)) {
    // msg.sensorKey = key sensor target (null = seluruh perangkat); jalankan msg.payload
    done.add(msg.commandId);
    client.publish(
      "devices/tester-water-sukawesi/cmd/ack",
      JSON.stringify({ commandId: msg.commandId, success: true }),
      { qos: 1 },
    );
  }
  if (topic.endsWith("/ota")) {
    if (done.has(msg.jobId)) return; // sudah selesai: jangan pasang ulang
    const report = (body) =>
      client.publish(
        "devices/tester-water-sukawesi/ota/status",
        typeof body === "string"
          ? body
          : JSON.stringify({ jobId: msg.jobId, ...body }),
        { qos: 1 },
      );
    report({ state: "received" });
    downloadAndFlash(msg.url) // fungsi buatan firmware
      .then(() => {
        done.add(msg.jobId);
        report({ success: true, message: `Firmware ${msg.version} terpasang` });
      })
      .catch((err) => report({ success: false, message: String(err) }));
    // pesan apa pun juga diterima sebagai ack, mis. report('OK') atau report('FAILED: flash error')
  }
  if (topic.endsWith("/ping"))
    client.publish(
      "devices/tester-water-sukawesi/pong",
      JSON.stringify({ pong: true }),
      { qos: 1 },
    );
});
```

### Arduino / ESP32 (PubSubClient)

```cpp
// setelah WiFi/GPRS terhubung
mqtt.setServer("localhost", 1883);
mqtt.connect("tester-water-sukawesi", "device_tester_water_sukawesi", "<MQTT_PASSWORD_TERDAFTAR>");
mqtt.subscribe("devices/tester-water-sukawesi/cmd", 1);   // QoS 1 -> pesan retained langsung datang (tanpa retain: hanya bila sedang online)
mqtt.subscribe("devices/tester-water-sukawesi/ota", 1);
mqtt.subscribe("devices/tester-water-sukawesi/ping", 1);
mqtt.publish("devices/tester-water-sukawesi/data", "24.6,81.2,6.8,3,1013.2,0.4,24.6,81.2,6.8,3");
mqtt.loop();            // beri waktu 10-30 detik untuk menerima instruksi sebelum tidur

// callback pesan masuk (mqtt.setCallback)
void onMessage(char* topic, byte* payload, unsigned int length) {
  if (length == 0) return;                       // pesan kosong = retained dikosongkan
  // parse JSON -> url, jobId, version; lewati jobId yang sudah selesai (simpan di flash/RTC)
  // t_httpUpdate_return r = httpUpdate.update(client, url);   // ESP32 HTTPUpdate
  // mqtt.publish("devices/tester-water-sukawesi/ota/status", r == HTTP_UPDATE_OK ? "OK" : "FAILED: update gagal");  // teks apa pun diterima
}
```

## 7. Debugging dengan Log Ingestion

Halaman **Log Ingestion** di dashboard mencatat setiap pembacaan yang diterima (`accepted`/`rejected`) beserta alasannya. Tombol **Payload** di tiap baris menampilkan isi pesan mentah yang dikirim stasiun (maks. 4 KB) — pakai untuk memeriksa urutan kolom dan format angka. Mode **Stream Live** menampilkan data `/stream` secara real-time. Untuk kalibrasi, buka halaman detail stasiun: bagian _Konfigurasi Sensor & Rumus Kalibrasi_ menampilkan nilai raw dan hasil rumus per sensor secara live selama stasiun mem-publish ke `/stream`.

| Pesan                                              | Penyebab umum                                                                                             |
| -------------------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| `Field N (sensor "x"): Cannot parse numeric value` | kolom N bukan angka (urutan kolom bergeser, atau desimal berkoma pada format koma)                        |
| `Unknown or inactive sensor key: x`                | `sensorKey` JSON tidak ada/nonaktif di stasiun                                                            |
| `Empty ingestion payload`                          | pesan kosong                                                                                              |
| (tidak ada log sama sekali, MQTT)                  | username/password salah, topic bukan milik stasiun ini (dibuang broker), atau nama stasiun di topic salah |

## 8. Analisis latensi komunikasi

Setiap jenis komunikasi mencatat durasinya sendiri untuk dianalisis lewat dashboard/API:

| Komunikasi                             | Field tersimpan                                         | Yang benar-benar diukur                                                                                                                                                                                             |
| -------------------------------------- | ------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Data masuk (station→server, REST/MQTT) | `IngestionLog.processingMs` (per pembacaan sensor)      | Waktu pemrosesan DI SISI SERVER saja (dari pesan diterima sampai nilai selesai diproses, termasuk waktu antre) — **bukan** waktu transit jaringan, karena payload data tidak membawa jam perangkat yang tersinkron. |
| Instruksi (server→station→server)      | `CommandLog.sentAt`, `ackAt`, `latencyMs`               | Waktu pulang-pergi: sejak server publish ke `/cmd` sampai `/cmd/ack` diterima kembali. Untuk stasiun yang tidur, ini juga mencakup waktu menunggu stasiun bangun.                                                   |
| OTA (server→station→server)            | `OtaUpdateJob.deliveredAt`, `completedAt`, `durationMs` | Waktu sejak OTA di-publish sampai device melaporkan hasil akhir — termasuk waktu menunggu device bangun serta unduh+pasang firmware.                                                                                |
| Ping (server→station→server)           | `PingTest.requestedAt`, `respondedAt`, `latencyMs`      | Waktu pulang-pergi murni `/ping`→`/pong`, indikator paling bersih untuk kondisi jaringan device saat ini.                                                                                                           |

Untuk mengukur latensi transit jaringan station→server yang sesungguhnya, perangkat perlu menyertakan jam perangkatnya sendiri di setiap pesan data — belum menjadi bagian dari format payload saat ini.
