// Data Collector for Environmental State Classifier
//
// Reads the thermistor, photoresistor, and DHT11, and prints one CSV row
// per sample to Serial. No labeling happens on the Arduino side -- labels
// get added afterward on the laptop, since you'll be physically creating
// conditions (cupping a hand over the photoresistor, breathing on the
// thermistor) while this runs, and it's easier to label a batch of rows
// after the fact than to wire up a labeling button mid-collection.
//
// Output format: raw_thermistor,raw_photoresistor,dht_temp_c,dht_humidity
//
// Usage: open Serial Monitor (or redirect to a file -- see the Python
// collector script for an automated version), and run one "session" per
// condition you're trying to capture, then label that whole batch in the
// CSV afterward.

#include <DHT.h>

const uint8_t THERMISTOR_PIN = A0;
const uint8_t PHOTORESISTOR_PIN = A1;
const uint8_t DHT_PIN = 8;  // matches deploy/env_classifier.ino -- D7 is reserved
                              // for the LCD's RS line in that sketch, so using D8
                              // here too means the same physical wiring works for
                              // both data collection and final deployment.

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

const unsigned long SAMPLE_INTERVAL_MS = 500;
unsigned long lastSampleMs = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();

  // Header row -- the Python training script expects this exact order.
  Serial.println("raw_thermistor,raw_photoresistor,dht_temp_c,dht_humidity");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  int thermistorRaw = analogRead(THERMISTOR_PIN);
  int photoRaw = analogRead(PHOTORESISTOR_PIN);

  // DHT11 is slow and occasionally returns NaN on a failed read. When that
  // happens we skip the row entirely rather than logging a garbage value --
  // a NaN in the training data would silently corrupt the dataset, and an
  // occasional dropped sample doesn't matter when collecting a few hundred
  // rows per condition.
  float dhtTemp = dht.readTemperature();
  float dhtHumidity = dht.readHumidity();
  if (isnan(dhtTemp) || isnan(dhtHumidity)) {
    return;
  }

  Serial.print(thermistorRaw);
  Serial.print(",");
  Serial.print(photoRaw);
  Serial.print(",");
  Serial.print(dhtTemp);
  Serial.print(",");
  Serial.println(dhtHumidity);
}
