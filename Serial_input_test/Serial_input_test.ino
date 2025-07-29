void setup() {
  // Initialize Serial (UART0) at the baud rate of your external device
  // Default pins for Serial (UART0) are RX = GPIO3, TX = GPIO1
  Serial.begin(115200); // Change to match your external device's baud rate

  Serial.println("ESP32 ready to receive serial data on UART0...");
}

void loop() {
  // Check if there's any data available to read on Serial (UART0)
  if (Serial.available()) {
    // Read the incoming byte
    char incomingByte = Serial.read();

    // Print the received byte to the Serial Monitor (which is also UART0)
    // This will echo the received data back to your Serial Monitor.
    Serial.print("Received on UART0: ");
    Serial.println(incomingByte);

    // You can add your processing logic here based on incomingByte
  }
}