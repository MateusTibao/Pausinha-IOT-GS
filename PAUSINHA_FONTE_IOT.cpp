#include <WiFi.h>              // Biblioteca nativa do ESP32 para conexão Wi-Fi
#include <PubSubClient.h>      // Biblioteca para comunicação MQTT (publish/subscribe)

// -------- Configuração de WiFi --------
const char* ssid       = "Wokwi-GUEST";
const char* password   = "";


bool wifiOk = false;

// -------- Configuração de MQTT --------
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

// -------- Limiares de decisão --------
// Valor abaixo do qual consideramos a luminosidade baixa
const int LIMIAR_LUZ_BAIXA      = 1500;
// Valor acima do qual consideramos o "estresse" alto (via potenciômetro)
const int LIMIAR_ESTRESSE_POT   = 2500;
// Tempo máximo sem movimento para considerar sedentarismo (20 segundos, em ms)
const unsigned long TEMPO_SEDENTARISMO_MS = 20000;

// Variável que guarda o instante do último movimento detectado (PIR)
unsigned long ultimoMovimentoMs = 0;

unsigned long ultimoToggleBuzzerMs = 0;
bool estadoBuzzer = false;

// Controle de logs periódicos no Serial (para debug)
unsigned long ultimoLogMs = 0;

// Tipo de pausa sugerida pelo sistema no momento
// 0 = nenhuma, 1 = descanso ocular, 2 = pausa ativa, 3 = respiracao guiada
int tipoPausaAtual = 0;

// Comando de pausa recebido via MQTT (se houver override remoto)
// -1 indica que não há comando ativo
int tipoPausaMQTT = -1;
// Momento em que o último comando MQTT foi recebido
unsigned long tempoUltimoComandoMQTT = 0;
// Tempo máximo que um comando MQTT permanece válido (15s)
const unsigned long TEMPO_COMANDO_MQTT_MS = 15000;

// ---------- Funções auxiliares de WiFi / MQTT ----------

// Realiza a conexão Wi-Fi e atualiza a flag wifiOk
void setup_wifi() {
  Serial.println();
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);             
  WiFi.begin(ssid, password);   

  unsigned long inicio = millis();
  // Aguarda a conexão por até 10 segundos, imprimindo pontos no Serial
  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 10000) {
    delay(200);
    Serial.print(".");
  }

  // Verifica se conectou com sucesso
  if (WiFi.status() == WL_CONNECTED) {
    wifiOk = true;
    Serial.println();
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());  // Exibe o IP obtido
  } else {
    wifiOk = false;
    Serial.println();
    Serial.println("Falha ao conectar no WiFi. Seguindo em modo offline (local apenas).");
  }
}

// Converte uma string recebida via MQTT em um código numérico de tipo de pausa
int mapaStringParaTipoPausa(const String& msg) {
  if (msg == "nenhuma")            return 0;
  if (msg == "descanso_ocular")    return 1;
  if (msg == "pausa_ativa")        return 2;
  if (msg == "respiracao_guiada")  return 3;
  return -1; // Retorna -1 se a mensagem não for reconhecida
}

// Função callback chamada automaticamente quando chega mensagem em um tópico inscrito
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  // Reconstrói a mensagem de texto a partir do payload (vetor de bytes)
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  // Log básico da mensagem recebida
  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  // Se a mensagem veio no tópico de comandos, interpretamos como tipo de pausa
  if (String(topic) == TOPICO_COMANDOS) {
    int tipo = mapaStringParaTipoPausa(msg);
    if (tipo >= 0) {
      // Salva o tipo de pausa remoto e registra o horário do comando
      tipoPausaMQTT = tipo;
      tempoUltimoComandoMQTT = millis();
      Serial.print("Comando remoto aplicado: ");
      Serial.println(msg);
    }
  }
}

// Tenta restabelecer a conexão com o broker MQTT de forma não bloqueante
void tentarReconectarMQTT() {
  // Só tenta se o WiFi estiver ok
  if (!wifiOk) return;
  // Se já está conectado ao broker, não faz nada
  if (client.connected()) return;

  // Controla o intervalo entre tentativas (a cada 5 segundos)
  static unsigned long ultimoTentativa = 0;
  if (millis() - ultimoTentativa < 5000) return; // tenta a cada 5s no máximo
  ultimoTentativa = millis();

  Serial.print("Tentando conectar ao MQTT... ");
  // Gera um clientId único para este ESP32
  String clientId = "PausinhaESP32-";
  clientId += String(random(0xffff), HEX);

  // Tenta conectar ao broker MQTT
  if (client.connect(clientId.c_str())) {
    Serial.println("conectado.");
    // Assim que conecta, se inscreve no tópico de comandos
    client.subscribe(TOPICO_COMANDOS);
    Serial.print("Inscrito no topico: ");
    Serial.println(TOPICO_COMANDOS);
  } else {
    // Se falhar, exibe o código de erro para debug
    Serial.print("falhou, rc=");
    Serial.println(client.state());
  }
}

// ---------- Lógica local de decisão de pausas ----------

// Converte o código numérico de tipo de pausa para a string correspondente
String nomePausa(int tipoPausa) {
  switch (tipoPausa) {
    case 0: return "nenhuma";
    case 1: return "descanso_ocular";
    case 2: return "pausa_ativa";
    case 3: return "respiracao_guiada";
    default: return "desconhecida";
  }
}

// Envia o estado atual (sensores + tipo de pausa) via Serial e MQTT
void enviarEstado(int tipoPausa, int valorLdr, int valorPot, bool movimento) {
  String pausaStr = nomePausa(tipoPausa);

  // Log legível no Serial Monitor (para testes e demo)
  Serial.print("LDR=");
  Serial.print(valorLdr);
  Serial.print(" | POT=");
  Serial.print(valorPot);
  Serial.print(" | MOV=");
  Serial.print(movimento ? "1" : "0");
  Serial.print(" | PAUSA=");
  Serial.println(pausaStr);

  // Se não há WiFi ou MQTT conectado, sai da função sem publicar
  if (!wifiOk || !client.connected()) {
    return;
  }

  // Monta o payload JSON com os dados dos sensores e da pausa atual
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

  // Publica a mensagem JSON no tópico de sensores
  client.publish(TOPICO_SENSORES, payload.c_str());
}

// Aplica a lógica de negócio para decidir qual pausa sugerir com base nos sinais locais
int decidirTipoDePausaLocal(int valorLdr, int valorPot, bool sedentario) {
  // Regra 1: se está sedentário há muito tempo → prioriza pausa ativa
  if (sedentario) {
    return 2; // pausa ativa
  }
  // Regra 2: se o nível de estresse está alto → respiração guiada
  if (valorPot > LIMIAR_ESTRESSE_POT) {
    return 3; // respiracao guiada
  }
  // Regra 3: se a luz está baixa → descanso ocular
  if (valorLdr < LIMIAR_LUZ_BAIXA) {
    return 1; // descanso ocular
  }
  // Caso contrário, nenhuma pausa é sugerida
  return 0;
}

// Atualiza os atuadores (LED e buzzer) de acordo com o tipo de pausa final
void atualizarAtuadores(int tipoPausa) {
  // Se não há pausa sugerida, desliga tudo
  if (tipoPausa == 0) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    estadoBuzzer = false;
    return;
  }

  // Para qualquer pausa ativa, mantém o LED aceso
  digitalWrite(LED_PIN, HIGH);

  // Faz o buzzer "piscar" (ligar/desligar) a cada 300 ms
  unsigned long agora = millis();
  if (agora - ultimoToggleBuzzerMs >= 300) {
    ultimoToggleBuzzerMs = agora;
    estadoBuzzer = !estadoBuzzer;
    digitalWrite(BUZZER_PIN, estadoBuzzer ? HIGH : LOW);
  }
}

// ---------- Funções principais: setup() e loop() ----------

void setup() {
  Serial.begin(115200);  // Inicializa a comunicação Serial para debug
  delay(100);            // Pequeno atraso para estabilizar

  // Define os modos de cada pino (entrada/saída)
  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(POT_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Garante que os atuadores começam desligados
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Inicializa o "último movimento" com o instante atual
  ultimoMovimentoMs = millis();

  Serial.println("Pausinha IoT - Sistema iniciado. Tentando WiFi...");
  // Tenta conectar ao WiFi
  setup_wifi();

  // Configura o servidor MQTT (endereço e porta)
  client.setServer(mqtt_server, mqtt_port);
  // Define a função callback para processar mensagens recebidas
  client.setCallback(mqttCallback);
}

void loop() {
  // 1) Gestão da conexão MQTT (não bloqueante)
  if (wifiOk) {
    tentarReconectarMQTT();    // Tenta reconectar se estiver desconectado
    if (client.connected()) {
      client.loop();           // Processa mensagens pendentes do broker
    }
  }

  // 2) Leitura dos sensores
  int valorLdr = analogRead(LDR_PIN);   // Lê a intensidade de luz (0 a 4095)
  int valorPot = analogRead(POT_PIN);   // Lê o "nível de estresse" (potenciômetro)
  int leituraPir = digitalRead(PIR_PIN); // Lê se há movimento (HIGH/LOW)

  // Converte leitura do PIR em booleano de movimento
  bool movimentoDetectado = (leituraPir == HIGH);
  // Sempre que houver movimento, atualiza o timestamp
  if (movimentoDetectado) {
    ultimoMovimentoMs = millis();
  }

  // 3) Cálculo de sedentarismo com base no tempo parado
  unsigned long agora = millis();
  bool sedentario = (agora - ultimoMovimentoMs) > TEMPO_SEDENTARISMO_MS;

  // 4) Decide o tipo de pausa local (sem considerar comandos remotos ainda)
  int sugestaoLocal = decidirTipoDePausaLocal(valorLdr, valorPot, sedentario);

  // 5) Aplica o override de comando MQTT, se ainda estiver dentro do tempo válido
  int novoTipoPausa = sugestaoLocal;
  if (tipoPausaMQTT >= 0 && (millis() - tempoUltimoComandoMQTT < TEMPO_COMANDO_MQTT_MS)) {
    novoTipoPausa = tipoPausaMQTT;
  }

  // 6) Se houve mudança no tipo de pausa, envia o novo estado
  if (novoTipoPausa != tipoPausaAtual) {
    tipoPausaAtual = novoTipoPausa;
    enviarEstado(tipoPausaAtual, valorLdr, valorPot, movimentoDetectado);
  }

  // 7) Log periódico de debug (a cada 500 ms)
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

  // 8) Atualiza LED e buzzer de acordo com o tipo de pausa final
  atualizarAtuadores(tipoPausaAtual);

  // Pequeno delay para reduzir a taxa de loop e evitar excesso de logs
  delay(100);
}
