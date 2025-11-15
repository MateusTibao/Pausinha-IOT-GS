#include <WiFi.h>
#include <PubSubClient.h>

// -------- WiFi --------
const char* ssid       = "Wokwi-GUEST";
const char* password   = "";

bool wifiOk = false;

// -------- MQTT --------
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;

const char* TOPICO_SENSORES = "pausinha/sensores";
const char* TOPICO_COMANDOS = "pausinha/comandos";

WiFiClient espClient;
PubSubClient client(espClient);

// -------- Pinos dos sensores --------
const int LDR_PIN       = 34; 
const int PIR_PIN       = 27; 
const int POT_PIN       = 32; 

// -------- Pinos dos atuadores --------
const int LED_PIN       = 25;
const int BUZZER_PIN    = 26;

// -------- Limiares --------
const int LIMIAR_LUZ_BAIXA      = 1500;
const int LIMIAR_ESTRESSE_POT   = 2500;
const unsigned long TEMPO_SEDENTARISMO_MS = 20000;

// Controle de tempo de movimento
unsigned long ultimoMovimentoMs = 0;

// Controle do buzzer em modo "pulsando"
unsigned long ultimoToggleBuzzerMs = 0;
bool estadoBuzzer = false;

// Log periódico
unsigned long ultimoLogMs = 0;

// Tipo de pausa sugerida
// 0 = nenhuma, 1 = descanso ocular, 2 = pausa ativa, 3 = respiracao guiada
int tipoPausaAtual = 0;

// Comando vindo do MQTT (override)
int tipoPausaMQTT = -1;
unsigned long tempoUltimoComandoMQTT = 0;
const unsigned long TEMPO_COMANDO_MQTT_MS = 15000;

// ---------- WiFi / MQTT helpers ----------

void setup_wifi() {
  Serial.println();
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 10000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiOk = true;
    Serial.println();
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiOk = false;
    Serial.println();
    Serial.println("Falha ao conectar no WiFi. Seguindo em modo offline (local apenas).");
  }
}

int mapaStringParaTipoPausa(const String& msg) {
  if (msg == "nenhuma")            return 0;
  if (msg == "descanso_ocular")    return 1;
  if (msg == "pausa_ativa")        return 2;
  if (msg == "respiracao_guiada")  return 3;
  return -1;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  if (String(topic) == TOPICO_COMANDOS) {
    int tipo = mapaStringParaTipoPausa(msg);
    if (tipo >= 0) {
      tipoPausaMQTT = tipo;
      tempoUltimoComandoMQTT = millis();
      Serial.print("Comando remoto aplicado: ");
      Serial.println(msg);
    }
  }
}

void tentarReconectarMQTT() {
  if (!wifiOk) return;
  if (client.connected()) return;

  static unsigned long ultimoTentativa = 0;
  if (millis() - ultimoTentativa < 5000) return; // tenta a cada 5s no máximo
  ultimoTentativa = millis();

  Serial.print("Tentando conectar ao MQTT... ");
  String clientId = "PausinhaESP32-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("conectado.");
    client.subscribe(TOPICO_COMANDOS);
    Serial.print("Inscrito no topico: ");
    Serial.println(TOPICO_COMANDOS);
  } else {
    Serial.print("falhou, rc=");
    Serial.println(client.state());
  }
}

// ---------- Lógica local ----------

String nomePausa(int tipoPausa) {
  switch (tipoPausa) {
    case 0: return "nenhuma";
    case 1: return "descanso_ocular";
    case 2: return "pausa_ativa";
    case 3: return "respiracao_guiada";
    default: return "desconhecida";
  }
}

void enviarEstado(int tipoPausa, int valorLdr, int valorPot, bool movimento) {
  String pausaStr = nomePausa(tipoPausa);

  Serial.print("LDR=");
  Serial.print(valorLdr);
  Serial.print(" | POT=");
  Serial.print(valorPot);
  Serial.print(" | MOV=");
  Serial.print(movimento ? "1" : "0");
  Serial.print(" | PAUSA=");
  Serial.println(pausaStr);

  if (!wifiOk || !client.connected()) {
    return;
  }

  String payload = "{";
  payload += "\"ldr\":";
  payload += valorLdr;
  payload += ",\"pot\":";
  payload += valorPot;
  payload += ",\"movimento\":";
  payload += (movimento ? "true" : "false");
  payload += ",\"pausa\":\"";
  payload += pausaStr;
  payload += "\"}";

  client.publish(TOPICO_SENSORES, payload.c_str());
}

int decidirTipoDePausaLocal(int valorLdr, int valorPot, bool sedentario) {
  if (sedentario) {
    return 2; // pausa ativa
  }
  if (valorPot > LIMIAR_ESTRESSE_POT) {
    return 3; // respiracao guiada
  }
  if (valorLdr < LIMIAR_LUZ_BAIXA) {
    return 1; // descanso ocular
  }
  return 0;
}

void atualizarAtuadores(int tipoPausa) {
  if (tipoPausa == 0) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    estadoBuzzer = false;
    return;
  }

  digitalWrite(LED_PIN, HIGH);

  unsigned long agora = millis();
  if (agora - ultimoToggleBuzzerMs >= 300) {
    ultimoToggleBuzzerMs = agora;
    estadoBuzzer = !estadoBuzzer;
    digitalWrite(BUZZER_PIN, estadoBuzzer ? HIGH : LOW);
  }
}

// ---------- Setup & Loop ----------

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(POT_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  ultimoMovimentoMs = millis();

  Serial.println("Pausinha IoT - Sistema iniciado. Tentando WiFi...");
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  // MQTT não bloqueante
  if (wifiOk) {
    tentarReconectarMQTT();
    if (client.connected()) {
      client.loop();
    }
  }

  // Leitura sensores
  int valorLdr = analogRead(LDR_PIN);
  int valorPot = analogRead(POT_PIN);
  int leituraPir = digitalRead(PIR_PIN);

  bool movimentoDetectado = (leituraPir == HIGH);
  if (movimentoDetectado) {
    ultimoMovimentoMs = millis();
  }

  unsigned long agora = millis();
  bool sedentario = (agora - ultimoMovimentoMs) > TEMPO_SEDENTARISMO_MS;

  int sugestaoLocal = decidirTipoDePausaLocal(valorLdr, valorPot, sedentario);

  int novoTipoPausa = sugestaoLocal;
  if (tipoPausaMQTT >= 0 && (millis() - tempoUltimoComandoMQTT < TEMPO_COMANDO_MQTT_MS)) {
    novoTipoPausa = tipoPausaMQTT;
  }

  if (novoTipoPausa != tipoPausaAtual) {
    tipoPausaAtual = novoTipoPausa;
    enviarEstado(tipoPausaAtual, valorLdr, valorPot, movimentoDetectado);
  }

  if (millis() - ultimoLogMs >= 500) {
    ultimoLogMs = millis();
    Serial.print("DEBUG | LDR=");
    Serial.print(valorLdr);
    Serial.print(" | POT=");
    Serial.print(valorPot);
    Serial.print(" | PIR=");
    Serial.print(movimentoDetectado ? "1" : "0");
    Serial.print(" | SED=");
    Serial.print(sedentario ? "1" : "0");
    Serial.print(" | LOCAL=");
    Serial.print(nomePausa(sugestaoLocal));
    Serial.print(" | MQTT=");
    if (tipoPausaMQTT >= 0 && (millis() - tempoUltimoComandoMQTT < TEMPO_COMANDO_MQTT_MS)) {
      Serial.print(nomePausa(tipoPausaMQTT));
    } else {
      Serial.print("sem_comando");
    }
    Serial.print(" | FINAL=");
    Serial.println(nomePausa(tipoPausaAtual));
  }

  atualizarAtuadores(tipoPausaAtual);

  delay(100);
}
