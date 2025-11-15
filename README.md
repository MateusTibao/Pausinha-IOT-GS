# Pausinha

O **Pausinha IoT** é um protótipo desenvolvido para a Global Solution 2025 – O Futuro do Trabalho.  
Ele representa o “dispositivo físico” do sistema Pausinha: um módulo com ESP32 que monitora o ambiente e o comportamento do usuário e sugere pausas inteligentes durante a jornada de trabalho.

A ideia é mostrar, na prática, como **IoT + IoB** podem apoiar a saúde e a produtividade de profissionais que passam horas em frente ao computador.

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

## 🖥️ Simulação Wokwi

Link para acesso: https://wokwi.com/projects/447553541199353857


---

## 📹 Video explicativo

Link para acesso: 


---

## 🔧 Componentes Utilizados

- 1x ESP32 DevKit V1  
- 1x Sensor LDR (módulo)  
- 1x Sensor PIR  
- 1x Potenciômetro  
- 1x LED  
- 1x Buzzer  

---

## 🌐 Comunicação MQTT

#Broker Público
- Broker TCP (ESP32): **broker.hivemq.com**
- Porta TCP (ESP32): **1883**
- Broker WebSocket (Dashboard / Browser): **mqtt-dashboard.com**
- Porta WebSocket: **8884**
- SSL: **ativado**
- Path WebSocket: **/mqtt**

### Publicação — `pausinha/sensores`

Exemplo:

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
6. (Opcional) Valide a comunicação MQTT conectando ao **HiveMQ WebSocket Client** e assinando os tópicos.

---

## 📡 Testando o MQTT com o HiveMQ Web Client (Opcional)

1. Acesse: https://www.hivemq.com/demos/websocket-client/?path=/mqtt
2. Configure a conexão:  
   - **Host:** `mqtt-dashboard.com`  
   - **Port:** `8884`  
   - **Path:** `/mqtt`  
   - **Client ID:** qualquer identificador único 
   - **SSL:** habilitado
3. Clique em **Connect**.  
4. Na aba **Subscribe**, adicione o tópico: pausinha/sensores

Você verá mensagens JSON sendo publicadas pelo ESP32 sempre que o tipo de pausa mudar.

5. Para enviar comandos ao dispositivo, vá em **Publish**:  
   - **Topic:** `pausinha/comandos`  
   - **Message:** um dos valores válidos:
     - `nenhuma`  
     - `descanso_ocular`  
     - `pausa_ativa`  
     - `respiracao_guiada`  

O ESP32 aplicará esse comando durante 15 segundos, demonstrando o modo de controle remoto/smart.

## Créditos
Um trabalho realizado pela equipe composta por Caio Hideki, Jorge Booz e Mateus Tibão.

