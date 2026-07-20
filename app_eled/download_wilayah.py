import requests
import json
import time
import os

BASE_URL = "https://wilayah.id/api"

OUTPUT_FILE = "indonesia_wilayah.json"


def get_data(url):
    print(f"Mengambil: {url}")

    response = requests.get(
        url,
        timeout=30
    )

    response.raise_for_status()

    return response.json()["data"]


def main():

    print("=" * 60)
    print("DOWNLOAD DATA WILAYAH INDONESIA")
    print("=" * 60)

    indonesia = {
        "provinces": []
    }

    # =====================================================
    # 1. PROVINSI
    # =====================================================

    provinces = get_data(
        f"{BASE_URL}/provinces.json"
    )

    print(
        f"Jumlah provinsi: "
        f"{len(provinces)}"
    )

    # =====================================================
    # 2. LOOP PROVINSI
    # =====================================================

    for province_index, province in enumerate(
        provinces,
        start=1
    ):

        province_code = province["code"]

        province_name = province["name"]

        print()
        print(
            f"[{province_index}/{len(provinces)}]"
            f" PROVINSI: {province_name}"
        )

        province_data = {
            "code": province_code,
            "name": province_name,
            "regencies": []
        }

        # =================================================
        # 3. KABUPATEN / KOTA
        # =================================================

        regencies = get_data(
            f"{BASE_URL}/regencies/"
            f"{province_code}.json"
        )

        for regency_index, regency in enumerate(
            regencies,
            start=1
        ):

            regency_code = regency["code"]

            regency_name = regency["name"]

            print(
                f"  [{regency_index}/"
                f"{len(regencies)}]"
                f" {regency_name}"
            )

            regency_data = {
                "code": regency_code,
                "name": regency_name,
                "districts": []
            }

            # =============================================
            # 4. KECAMATAN
            # =============================================

            districts = get_data(
                f"{BASE_URL}/districts/"
                f"{regency_code}.json"
            )

            for district_index, district in enumerate(
                districts,
                start=1
            ):

                district_code = district["code"]

                district_name = district["name"]

                print(
                    f"    [{district_index}/"
                    f"{len(districts)}]"
                    f" {district_name}"
                )

                district_data = {
                    "code": district_code,
                    "name": district_name,
                    "villages": []
                }

                # =========================================
                # 5. KELURAHAN / DESA
                # =========================================

                villages = get_data(
                    f"{BASE_URL}/villages/"
                    f"{district_code}.json"
                )

                for village in villages:

                    village_data = {
                        "code": village["code"],
                        "name": village["name"]
                    }

                    district_data[
                        "villages"
                    ].append(
                        village_data
                    )

                regency_data[
                    "districts"
                ].append(
                    district_data
                )

                time.sleep(0.05)

            province_data[
                "regencies"
            ].append(
                regency_data
            )

        indonesia[
            "provinces"
        ].append(
            province_data
        )

    # =====================================================
    # 6. SIMPAN JSON
    # =====================================================

    with open(
        OUTPUT_FILE,
        "w",
        encoding="utf-8"
    ) as file:

        json.dump(
            indonesia,
            file,
            ensure_ascii=False,
            indent=2
        )

    print()
    print("=" * 60)
    print(
        "SELESAI!"
    )

    print(
        f"File berhasil dibuat:"
        f" {OUTPUT_FILE}"
    )

    print("=" * 60)


if __name__ == "__main__":

    main()