<img width="3000" alt="logo horizontal" src="https://github.com/user-attachments/assets/feb01b80-17bb-4167-a3b9-6687f8ac4097" />

---

O Pausinha nasceu originalmente como um software de bem-estar corporativo, projetado para sugerir pausas ativas, alongamentos, hidratação e exercícios de respiração ao longo da jornada de trabalho. A proposta central sempre foi promover saúde, reduzir os efeitos do sedentarismo e equilibrar produtividade e bem-estar em ambientes digitais.

Para a Global Solution 2025 – O Futuro do Trabalho, essa ideia foi estendida para o universo de IoT, transformando o conceito do aplicativo em um dispositivo físico inteligente. Assim surgiu o Pausinha IoT: um módulo baseado em ESP32 capaz de monitorar o ambiente e o comportamento do usuário em tempo real, aplicando princípios de IoT + IoB (Internet of Behaviors) para sugerir pausas inteligentes.

Nessa versão, sensores e atuadores permitem que o sistema registre luminosidade, movimento e sinais de estresse, tome decisões automáticas e envie dados para análise via MQTT, demonstrando como tecnologias de automação podem reforçar saúde, ergonomia e qualidade de vida no futuro do trabalho.

---

## 🧩 Problema Abordado

Profissionais que trabalham em frente ao computador tendem a permanecer sentados por longos períodos, em ambientes nem sempre bem iluminados e sob alta pressão de prazos e entregas.  
Esse cenário favorece:

- **Sedentarismo e fadiga física**, com dores musculares e desconforto postural;
- **Cansaço visual**, devido a iluminação inadequada e uso intenso de telas;
- **Estresse e sobrecarga mental**, que impactam foco e produtividade.

No contexto do **futuro do trabalho**, em que o home office e os ambientes híbridos se tornaram comuns, torna-se essencial contar com tecnologias que **monitorem sinais de risco** e **sugiram pausas ativas** para preservar a saúde sem depender apenas da disciplina do colaborador.

---

## 🎯 Objetivo da Solução

Desenvolver um módulo IoT com **ESP32**, sensores e atuadores que:

1. Monitore luminosidade, movimento e “nível de estresse” do usuário;
2. Classifique o contexto e sugira o tipo de pausa mais adequado;
3. Notifique o usuário por meio de **feedback visual (LED)** e **sonoro (buzzer)**;
4. Envie os dados via **MQTT** para permitir dashboards, análise de comportamento e possíveis integrações com o sistema Pausinha (app, IA, etc.).

---

## 🌐 Conceito da Solução

O dispositivo coleta sinais de:

- **LDR (luminosidade)** → avalia se a iluminação está adequada para o trabalho.
- **PIR (movimento)** → detecta presença e ajuda a identificar sedentarismo prolongado.
- **Potenciômetro** → simula o nível de estresse / carga de trabalho do usuário.

Com base nesses dados, o ESP32 classifica o contexto e sugere um tipo de pausa:

- `nenhuma` – tudo dentro do normal  
- `descanso_ocular` – iluminação baixa  
- `pausa_ativa` – sedentarismo prolongado  
- `respiracao_guiada` – estresse alto  

Quando uma pausa é recomendada:

- **LED** acende → feedback visual  
- **Buzzer** apita de forma pulsante → feedback sonoro  

Além disso, o módulo envia os dados via **MQTT** para integração com dashboards, app web ou serviços de IA do Pausinha.

---

## ⚙️ Lógica de Decisão de Pausas

A lógica é baseada em regras simples de negócio:

1. **Sedentarismo**  
   - Se o sensor PIR não detecta movimento por mais de **20 segundos**, o sistema considera o usuário sedentário.  
   - Nessa condição, a pausa sugerida é `pausa_ativa` (levantar, alongar, caminhar).

2. **Estresse / Carga de trabalho**  
   - Se o valor lido no potenciômetro ultrapassa o limiar configurado (`LIMIAR_ESTRESSE_POT`), o sistema assume alto nível de estresse.  
   - A pausa sugerida passa a ser `respiracao_guiada`.

3. **Luminosidade inadequada**  
   - Se o valor do LDR fica abaixo de `LIMIAR_LUZ_BAIXA`, o ambiente é considerado escuro para trabalho de tela.  
   - A recomendação é `descanso_ocular`.

4. **Controle remoto via MQTT (override)**  
   - O tópico `pausinha/comandos` permite que um dashboard ou outro serviço envie diretamente um tipo de pausa (`nenhuma`, `descanso_ocular`, `pausa_ativa`, `respiracao_guiada`).  
   - Esse comando remoto tem prioridade sobre a lógica local por **15 segundos**, simulando um “modo smart” controlado pela camada de aplicação.

---

## 🖥️ Simulação Wokwi

Toda a solução foi implementada e testada em simulação, utilizando o ambiente **Wokwi**.

<img width="704" height="488" alt="image" src="https://github.com/user-attachments/assets/a580f662-1340-4f7c-bd12-ce670df9e1a0" />

- **Link da simulação:** [https://wokwi.com/projects/447553541199353857](https://wokwi.com/projects/447553541199353857)


---

## 📹 Vídeo Explicativo

- **Link para acesso:** https://youtu.be/vATxM0fH5uI

No vídeo são apresentados:

- O problema de sedentarismo e fadiga no contexto do futuro do trabalho;  
- O circuito no Wokwi em funcionamento (sensores, LED, buzzer);  
- A comunicação MQTT com o HiveMQ Web Client;  
- Os impactos esperados na rotina do colaborador e na gestão de bem-estar pelas empresas.

---

## 🔧 Componentes Utilizados

- 1x ESP32 DevKit V1  
- 1x Sensor LDR (módulo)  
- 1x Sensor PIR  
- 1x Potenciômetro  
- 1x LED  
- 1x Buzzer  

---

## 🧱 Dependências e Ambiente

- **Placa:** ESP32 DevKit V1 (simulado no Wokwi)  
- **Linguagem:** C++ (Arduino/ESP32)  
- **Bibliotecas utilizadas:**
  - `WiFi.h` – gerenciamento de conexão Wi-Fi;
  - `PubSubClient.h` – cliente MQTT para ESP32.

No Wokwi, essas bibliotecas já estão disponíveis por padrão.  
Em um ambiente físico/Arduino IDE, é necessário instalar a biblioteca **PubSubClient** via Library Manager.

---

## 🌐 Comunicação MQTT

### Broker Público

- **Broker TCP (ESP32):** `broker.hivemq.com`  
- **Porta TCP (ESP32):** `1883`  
- **Broker WebSocket (Dashboard / Browser):** `mqtt-dashboard.com`  
- **Porta WebSocket:** `8884`  
- **SSL:** ativado  
- **Path WebSocket:** `/mqtt`

### Publicação — `pausinha/sensores`

Exemplo de payload JSON:

```json
{
  "ldr": 2043,
  "pot": 2870,
  "movimento": true,
  "pausa": "pausa_ativa"
}
```

### Comandos recebidos — `pausinha/comandos`

Valores válidos:

```
nenhuma
descanso_ocular
pausa_ativa
respiracao_guiada
```

---

## ▶️ Como Executar no Wokwi

1. Abra o link público do circuito no Wokwi.  
2. Verifique que o WiFi do simulador está ativado e que o SSID configurado no código é **`Wokwi-GUEST`**.  
3. Inicie a simulação.  
4. Interaja com os sensores para visualizar as mudanças no tipo de pausa:
   - **LDR** → ajuste a intensidade de luz para simular ambientes claros/escuros.  
   - **PIR** → mova o cursor na frente do sensor para detectar presença.  
   - **Potenciômetro** → gire o knob para simular aumento de estresse.  
5. Observe no console e no LED/Buzzer como o dispositivo reage aos diferentes cenários.  
6. Valide a comunicação MQTT conectando ao **HiveMQ WebSocket Client** e assinando os tópicos.

---

## 📡 Testando o MQTT com o HiveMQ Web Client 

1. Acesse: https://www.hivemq.com/demos/websocket-client/?path=/mqtt  
2. Configure a conexão:  
   - **Host:** `mqtt-dashboard.com`  
   - **Port:** `8884`  
   - **Path:** `/mqtt`  
   - **Client ID:** qualquer identificador único  
   - **SSL:** habilitado  
3. Clique em **Connect**.  
4. Na aba **Subscribe**, adicione o tópico: `pausinha/sensores`.  
   - Você verá mensagens JSON sendo publicadas pelo ESP32 sempre que o tipo de pausa mudar.
5. Para enviar comandos ao dispositivo, vá em **Publish**:  
   - **Topic:** `pausinha/comandos`  
   - **Message:** um dos valores válidos:
     - `nenhuma`  
     - `descanso_ocular`  
     - `pausa_ativa`  
     - `respiracao_guiada`  

O ESP32 aplicará esse comando durante aproximadamente **15 segundos**, demonstrando o modo de controle remoto/smart.

---

## 🌍 Impacto no Futuro do Trabalho

O Pausinha IoT demonstra como um módulo simples de sensores e atuadores pode atuar como **agente de cuidado** dentro do ambiente de trabalho:

- Incentiva pausas ativas, reduzindo o tempo de sedentarismo contínuo;
- Contribui para diminuição de fadiga visual e sobrecarga mental;
- Gera dados que podem ser usados em **dashboards de bem-estar** e programas de saúde corporativa;
- Serve como prova de conceito de uma arquitetura **IoT + IoB + IA**, alinhada às demandas de ambientes híbridos, trabalho remoto e cultura de bem-estar.

---

## 👨‍💻 Créditos

Trabalho realizado pela equipe:

- **Caio Hideki (553630)**  
- **Jorge Booz (552700)**  
- **Mateus Tibão (553267)**
