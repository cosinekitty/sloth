/*
    VoltageReader.ino  -  Don Cross  -  2023-09-12

    Logs voltages as a function of time from the Sloth circuit.
    This allows me to compare the behavior of my simulated Sloth
    with the real thing.
*/

const unsigned long IntervalMillis = 200;
unsigned long TargetMillis;

void setup()
{
    Serial.begin(115200);
    TargetMillis = millis() + IntervalMillis;
}

void loop()
{
    unsigned long now;
    do { now = millis(); } while (now != TargetMillis);     // risky but should be OK
    TargetMillis = now + IntervalMillis;

    int vx = analogRead(A0);
    int vy = analogRead(A1);
    int vz = analogRead(A2);
    int vw = analogRead(A3);

    Serial.print(now);
    Serial.print(",");
    Serial.print(vx);
    Serial.print(",");
    Serial.print(vy);
    Serial.print(",");
    Serial.print(vz);
    Serial.print(",");
    Serial.print(vw);
    Serial.println();
}
