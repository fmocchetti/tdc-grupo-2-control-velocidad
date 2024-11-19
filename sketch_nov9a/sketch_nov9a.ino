// Pines de conexión brazo
const int motorAvancePin1 = 10;
const int motorAvancePin2 = 11;

const int buttonPin = 12;

// Constantes de control
const int velocidadMinima = 70; // Velocidad mínima para que el motor se mueva
const int velocidadMaxima = 125; // Velocidad máxima del motor
float K = 1.8; // Constante de proporcionalidad, ajustar según necesidad

// Pines y constantes para el sensor ultrasónico
#define TRIG_PIN 2
#define ECHO_PIN 3
const int distanciaParada = 20*10;  // Distancia en cm para detener el robot si hay un objeto


// Variables de estado
bool robotActivo = false;
int buttonState = HIGH;
int lastButtonState = HIGH;

void setup() {
  pinMode(motorAvancePin1, OUTPUT);
  pinMode(motorAvancePin2, OUTPUT);
  
  pinMode(buttonPin, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  //Serial.println("Control de brazo con balance de sensores y avance.");
}

int intentos = 20;

void loop() {
  // Leer el estado del botón y alternar el estado del robot
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(100);  // Anti-rebote
    robotActivo = !robotActivo;
    if (robotActivo) {
      //Serial.println("Robot activado.");
    } else {
      detenerMotorAvance();
      //Serial.println("Robot desactivado.");
    }
  }

  lastButtonState = buttonState;
  int distancia = medirDistancia();

  // Solo funciona si el robot está activo
  if (robotActivo) {
    // Verificar la distancia con el sensor ultrasónico
    int distancia = medirDistancia();
    if (distancia < distanciaParada) {
      detenerMotorAvance();
      //Serial.println("Obstáculo detectado, robot detenido.");
      robotActivo = false;  // Desactiva el robot al detectar el obstáculo
      return;
    } else if (distancia > distanciaParada) {
      //int velocidadAvance = map(distancia, distanciaParada, 100, 70, 200); 
      // Multiplicar el error por una K y variable de ajuste (si es necesario)
      //pwm = pwm_min + k * error; // El rango de valores de k debe elegirse de forma tal que K * el error < 255 

      int error = distancia - distanciaParada; // Calculamos el error con respecto a la distancia de parada
      int pwm = velocidadMinima + K * error;

      // Asegurarse de que PWM esté dentro de los límites permitidos
      pwm = constrain(pwm, velocidadMinima, velocidadMaxima);
      avanzar(pwm);
    }
  }

  if(intentos == 100) {
    Serial.println(distancia);
    //mandarDistancia(distancia, distanciaParada);
    intentos = 0;
  }

  intentos++;    
}

void avanzar(int velocidad) {
  analogWrite(motorAvancePin1, velocidad);
  digitalWrite(motorAvancePin2, LOW);
  //Serial.print("Avanzando con velocidad: ");
  //Serial.println(velocidad);
}

void retroceder(int velocidad) {
  analogWrite(motorAvancePin2, velocidad);
  digitalWrite(motorAvancePin1, LOW);
  //Serial.print("Retrocediendo con velocidad: ");
  //Serial.println(velocidad);
}

void detenerMotorAvance() {
  digitalWrite(motorAvancePin1, LOW);
  digitalWrite(motorAvancePin2, LOW);
  //Serial.println("Motor de avance detenido.");
}

int medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH);
  float distancia = (duracion * 0.0343 / 2)*10;

 /* Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm.");*/

  return (int) distancia;
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
