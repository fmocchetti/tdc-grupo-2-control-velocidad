// Pines de conexión brazo
const int motorAvancePin1 = 10;
const int motorAvancePin2 = 11;

const int buttonPin = 12;

// Constantes de control
const int velocidadMinima = 100; // Velocidad mínima para que el motor se mueva
const int velocidadMaxima = 200; // Velocidad máxima del motor
float K = 0.2; // Constante de proporcionalidad, ajustar según necesidad

// Pines y constantes para el sensor ultrasónico
#define TRIG_PIN 2
#define ECHO_PIN 3
int distanciaParada = 20*10;  // Distancia en cm para detener el robot si hay un objeto

const int builtInLedPin = LED_BUILTIN;

// Variables de estado
bool robotActivo = false;
int buttonState = HIGH;
int lastButtonState = HIGH;

#define NUM_READINGS 5  // Número de lecturas para el filtro de mediana

unsigned long lastMillis = 0;  // Guarda la última vez que se realizó la acción
const long interval = 100;    // Intervalo de tiempo para las acciones (1000 ms = 1 segundo)

void setup() {
  pinMode(motorAvancePin1, OUTPUT);
  pinMode(motorAvancePin2, OUTPUT);
  
  pinMode(builtInLedPin, OUTPUT);
  
  pinMode(buttonPin, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  //Serial.println("Control de brazo con balance de sensores y avance.");
  digitalWrite(builtInLedPin, LOW);
}
void loop() {
  // Leer el estado del botón y alternar el estado del robot
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(100);  // Anti-rebote
    robotActivo = !robotActivo;
    if (robotActivo) {
      digitalWrite(builtInLedPin, HIGH);
      //Serial.println("Robot activado.");
    } else {
      detenerMotorAvance();
      digitalWrite(builtInLedPin, LOW);
      //Serial.println("Robot desactivado.");
    }
  }

  lastButtonState = buttonState;
  int distancia = medirDistancia();

  // Solo funciona si el robot está activo
  if (robotActivo) {

    int error = distancia - distanciaParada; // Calculamos el error con respecto a la distancia de parada
    // Multiplicar el error por una K y variable de ajuste (si es necesario)
    int pwm = velocidadMinima + K * abs(error);
    pwm = constrain(pwm, velocidadMinima, velocidadMaxima);
    // Verificar la distancia con el sensor ultrasónico
    if (error > 0) {
      //detenerMotorAvance();
      digitalWrite(builtInLedPin, LOW);
      avanzar(pwm);
      //robotActivo = false;  // Desactiva el robot al detectar el obstáculo
    } else if (error < 0) {
      // Asegurarse de que PWM esté dentro de los límites permitidos     
      retroceder(pwm);
    } else if (error == 0) {
      detenerMotorAvance();
    }
  }
    // Verificar si ha pasado el tiempo suficiente desde la última vez que se envió la distancia
  if (millis() - lastMillis >= interval) {
    lastMillis = millis();  // Actualizar la última vez que se realizó la acción
    Serial.println(distancia);
    //mandarDistancia(distancia, distanciaParada);
  }

  // Leer y procesar el comando serial
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    parseCommand(input);
  }
}

void avanzar(int velocidad) {
  analogWrite(motorAvancePin1, velocidad);
  digitalWrite(motorAvancePin2, LOW);
}

void retroceder(int velocidad) {
  analogWrite(motorAvancePin2, velocidad);
  digitalWrite(motorAvancePin1, LOW);
}

void detenerMotorAvance() {
  digitalWrite(motorAvancePin1, LOW);
  digitalWrite(motorAvancePin2, LOW);
}

int readings[NUM_READINGS];    // array de lecturas del sensor
int readIndex = 0;             // índice de la lectura actual
int totalReadings = 0;         // número total de lecturas almacenadas, max NUM_READINGS

int medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = (int)(duration * 0.0343 / 2);

  // Guarda la lectura en el array y actualiza el índice circular
  readings[readIndex] = distance * 10;
  readIndex = (readIndex + 1) % NUM_READINGS;

  // Actualiza el contador de lecturas hasta alcanzar el número máximo
  if (totalReadings < NUM_READINGS) {
    totalReadings++;
  }

  // Calcula y retorna la mediana de las lecturas
  return calculateMedian(readings, totalReadings);
}

int calculateMedian(int arr[], int numElements) {
  int sortedArray[NUM_READINGS];  // array temporal para guardar las lecturas ordenadas
  memcpy(sortedArray, arr, numElements * sizeof(int));  // copia para no modificar el original
  // Ordena el array
  insertionSort(sortedArray, numElements);
  // Calcula la mediana
  if (numElements % 2 == 0) {
    return (sortedArray[numElements/2 - 1] + sortedArray[numElements/2]) / 2;
  } else {
    return sortedArray[numElements/2];
  }
}

void insertionSort(int arr[], int numElements) {
  for (int i = 1; i < numElements; i++) {
    int key = arr[i];
    int j = i - 1;
    // Mueve los elementos de arr[0..i-1], que son mayores que key, a una posición adelante
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
}

byte dataOut[10];

void mandarDistancia(int mm, int obj)
{
    dataOut[0] = 255;
    dataOut[1] = (mm >> 24) & 0xFF;
    dataOut[2] = (mm >> 16) & 0xFF;
    dataOut[3] = (mm >> 8) & 0xFF;
    dataOut[4] = mm & 0xFF;

    dataOut[5] = (obj >> 24) & 0xFF;
    dataOut[6] = (obj >> 16) & 0xFF;
    dataOut[7] = (obj >> 8) & 0xFF;
    dataOut[8] = obj & 0xFF;
    
    dataOut[9] = 255;
    Serial.write(dataOut,10);
}

void parseCommand(String command) {
  int kIndex = command.indexOf("K=");
  int distIndex = command.indexOf("DIST=");

  if (kIndex != -1) {
    int endIndex = command.indexOf(';', kIndex);
    String kValue = command.substring(kIndex + 2, endIndex);
    K = kValue.toFloat();
    Serial.print("Nueva K: ");
    Serial.println(K);
  }

  if (distIndex != -1) {
    int endIndex = command.indexOf(';', distIndex);
    String distValue = command.substring(distIndex + 5, endIndex);
    distanciaParada = distValue.toInt() * 10;  // Convertir cm a mm
    Serial.print("Nueva distancia de parada: ");
    Serial.println(distanciaParada);
  }
}
