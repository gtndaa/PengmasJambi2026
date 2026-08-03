import 'package:flutter/material.dart';

class BmkgScreen
    extends StatefulWidget {
  const BmkgScreen({
    super.key,
  });

  @override
  State<BmkgScreen> createState() =>
      _BmkgScreenState();
}

class _BmkgScreenState
    extends State<BmkgScreen> {
  // =====================================================
  // DATA SENSOR ESP32
  // =====================================================

  // Untuk sementara belum ada data ESP32.
  // Nilai akan diganti ketika API/cloud sudah tersedia.

  String temperature = '--';
  String humidity = '--';
  String soilMoisture = '--';
  String lightIntensity = '--';

  String deviceStatus =
      'Belum terhubung';

  String lastUpdate =
      'Belum ada data';

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      appBar: AppBar(
        title:
            const Text(
          'Data Lahan',
        ),
      ),

      body:
          SingleChildScrollView(
        padding:
            const EdgeInsets.all(
          16,
        ),

        child:
            Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .start,

          children: [
            _buildHeader(),

            const SizedBox(
              height: 20,
            ),

            _buildDeviceStatus(),

            const SizedBox(
              height: 20,
            ),

            _buildSensorGrid(),

            const SizedBox(
              height: 20,
            ),

            _buildInformationCard(),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // HEADER
  // =====================================================

  Widget _buildHeader() {
    return Column(
      crossAxisAlignment:
          CrossAxisAlignment.start,

      children: [
        const Text(
          'Data Lahan',

          style:
              TextStyle(
            fontSize: 28,
            fontWeight:
                FontWeight.bold,
          ),
        ),

        const SizedBox(
          height: 8,
        ),

        Text(
          'Pantau kondisi lahan '
          'berdasarkan data sensor.',

          style:
              TextStyle(
            color:
                Colors.grey.shade600,

            fontSize: 15,
          ),
        ),
      ],
    );
  }

  // =====================================================
  // STATUS DEVICE
  // =====================================================

  Widget _buildDeviceStatus() {
    return Card(
      elevation: 2,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          18,
        ),
      ),

      child:
          Padding(
        padding:
            const EdgeInsets.all(
          18,
        ),

        child:
            Row(
          children: [
            Container(
              width: 50,
              height: 50,

              decoration:
                  BoxDecoration(
                shape:
                    BoxShape.circle,

                color:
                    Colors.grey
                        .shade200,
              ),

              child:
                  const Icon(
                Icons.memory,
                size: 28,
              ),
            ),

            const SizedBox(
              width: 16,
            ),

            Expanded(
              child:
                  Column(
                crossAxisAlignment:
                    CrossAxisAlignment
                        .start,

                children: [
                  const Text(
                    'Status Perangkat',

                    style:
                        TextStyle(
                      fontWeight:
                          FontWeight.bold,

                      fontSize: 16,
                    ),
                  ),

                  const SizedBox(
                    height: 4,
                  ),

                  Text(
                    deviceStatus,

                    style:
                        TextStyle(
                      color:
                          Colors.grey
                              .shade600,
                    ),
                  ),
                ],
              ),
            ),

            const Icon(
              Icons.circle,
              size: 14,
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // SENSOR GRID
  // =====================================================

  Widget _buildSensorGrid() {
    return GridView.count(
      crossAxisCount: 2,

      shrinkWrap: true,

      physics:
          const NeverScrollableScrollPhysics(),

      crossAxisSpacing: 12,

      mainAxisSpacing: 12,

      childAspectRatio: 1.15,

      children: [
        _buildSensorCard(
          icon:
              Icons.thermostat,

          title:
              'Temperatur',

          value:
              temperature,

          unit:
              '°C',
        ),

        _buildSensorCard(
          icon:
              Icons.water_drop,

          title:
              'Kelembapan Udara',

          value:
              humidity,

          unit:
              '%',
        ),

        _buildSensorCard(
          icon:
              Icons.grass,

          title:
              'Kelembapan Tanah',

          value:
              soilMoisture,

          unit:
              '%',
        ),

        _buildSensorCard(
          icon:
              Icons.wb_sunny,

          title:
              'Intensitas Cahaya',

          value:
              lightIntensity,

          unit:
              'lux',
        ),
      ],
    );
  }

  // =====================================================
  // SENSOR CARD
  // =====================================================

  Widget _buildSensorCard({
    required IconData icon,

    required String title,

    required String value,

    required String unit,
  }) {
    return Card(
      elevation: 2,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          18,
        ),
      ),

      child:
          Padding(
        padding:
            const EdgeInsets.all(
          16,
        ),

        child:
            Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .start,

          mainAxisAlignment:
              MainAxisAlignment
                  .spaceBetween,

          children: [
            Icon(
              icon,
              size: 30,
            ),

            Column(
              crossAxisAlignment:
                  CrossAxisAlignment
                      .start,

              children: [
                Text(
                  title,

                  style:
                      TextStyle(
                    color:
                        Colors.grey
                            .shade600,

                    fontSize: 13,
                  ),
                ),

                const SizedBox(
                  height: 4,
                ),

                Row(
                  crossAxisAlignment:
                      CrossAxisAlignment
                          .end,

                  children: [
                    Text(
                      value,

                      style:
                          const TextStyle(
                        fontSize: 25,

                        fontWeight:
                            FontWeight.bold,
                      ),
                    ),

                    const SizedBox(
                      width: 4,
                    ),

                    Text(
                      unit,

                      style:
                          TextStyle(
                        color:
                            Colors.grey
                                .shade600,

                        fontSize: 12,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // INFORMATION CARD
  // =====================================================

  Widget _buildInformationCard() {
    return Card(
      elevation: 2,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          18,
        ),
      ),

      child:
          Padding(
        padding:
            const EdgeInsets.all(
          18,
        ),

        child:
            Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .start,

          children: [
            const Text(
              'Informasi Data',

              style:
                  TextStyle(
                fontSize: 18,
                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 12,
            ),

            _buildInfoRow(
              'Status data',
              'Belum tersedia',
            ),

            _buildInfoRow(
              'Pembaruan terakhir',
              lastUpdate,
            ),

            const SizedBox(
              height: 12,
            ),

            Container(
              width:
                  double.infinity,

              padding:
                  const EdgeInsets.all(
                12,
              ),

              decoration:
                  BoxDecoration(
                color:
                    Colors.grey
                        .shade100,

                borderRadius:
                    BorderRadius.circular(
                  10,
                ),
              ),

              child:
                  const Text(
                'Data sensor ESP32 akan '
                'ditampilkan setelah perangkat '
                'terhubung dengan sistem cloud.',
              ),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // INFO ROW
  // =====================================================

  Widget _buildInfoRow(
    String label,
    String value,
  ) {
    return Padding(
      padding:
          const EdgeInsets.symmetric(
        vertical: 5,
      ),

      child:
          Row(
        mainAxisAlignment:
            MainAxisAlignment
                .spaceBetween,

        children: [
          Text(
            label,

            style:
                TextStyle(
              color:
                  Colors.grey
                      .shade600,
            ),
          ),

          Text(
            value,

            style:
                const TextStyle(
              fontWeight:
                  FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }
}