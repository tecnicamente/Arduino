

unsigned char FBusFrame[200];

// Numero del centro messaggi del gestore
// WIND     +393205858500 
// Vodafone +393492000200
// TIM      +393359609600
// TRE      +393916263333
// Dopo 0x91 si deve inserire il numero del centro messaggi in formato esadecimale a coppie di cifre invertire
// ES: WIND +39 -> 0x93 ; 32 -> 0x23 ; 05 -> 0x50 ; 58 -> 0x85 ; 58 -> 0x85 ; 0 -> 0x00 ; 0 -> 0x00
// Il nokia si attende un numero composto da 20 numeri, quindi quindi essendo in Italia il formato composto da 12 numeri
// occorre inserire 8 zeri finali, che si traduce in 4 0x00
unsigned char SMSC[] = {0x07, 0x91, 0x93, 0x23, 0x50, 0x58, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00};

// Numero che riceve l'SMS. dopo 0x81 inserire il numero del destinatario come già fatto per il centro messaggi
// non occore inserire gli 0x00 di chiusura, il formato è libero
// Es: 0123456789 
unsigned char RecipientNo[] = {0x0A, 0x81, 0x10, 0x32, 0x54, 0x76, 0x98};

void setup() { 
  // inizializzo la comunicazione su seriale a 115200 kbs come richiesto dal protocollo F-bus
  Serial.begin(115200);
  delay(100);
} 

void loop() { 
  // Inizializzazione dell'F-Bus inviando 128 volte il carattere 'U', corrispondente ad un segnale '0101010101...'
  for (int x = 0; x < 128; x++) {
    Serial.write("U");
  } 

  // Invio l'SMS
  SendSMS("EVVAI, il primo SMS");

  // Leggo la risposta dal Nokia, questa parte va estesa con l'individuazione e la decodifica di eventuali SMS di riposta ricevuti dal Nokia
  while (1) {
    while (Serial.available() > 0) { 
      int incomingByte = Serial.read(); 
      Serial.print(incomingByte, HEX); 
      Serial.print(" "); 
    }
  }
} 

unsigned char SendSMS(const char *Message) {
  // 
  unsigned char j = 0;
  memset(FBusFrame, 0, sizeof(FBusFrame));

  unsigned char MsgLen = strlen(Message), FrameSize = 0;
  unsigned char c, w, n, shift = 0, frameIndex = 0;
  unsigned char oddCheckSum, evenCheckSum = 0, SeqNo = 0x43;
  unsigned char MsgStartIndex = 48; // Indice di inizio dell'SMS

  // Codifica in 7 bit
  for (n = 0; n < MsgLen; n++) {
    c = Message[n] & 0x7f;
    c >>= shift;
    w = Message[n+1] & 0x7f;
    w <<= (7-shift);
    shift += 1;
    c = c | w;
    if (shift == 7) {
      shift = 0x00;
      n++;
    }
    FBusFrame[frameIndex + MsgStartIndex] = c;
    frameIndex++;
  }

  FBusFrame[frameIndex + MsgStartIndex] = 0x01;
  FrameSize = frameIndex + 44; // The size of the frame is frameIndex+48 (FrameIndex + 48 + 1 - 5)

  // Preparazione del frame prima del contenuto dell'SMS secondo il protocollo F-bus 
  FBusFrame[0] = 0x1E;
  FBusFrame[1] = 0x00;
  FBusFrame[2] = 0x0C;
  FBusFrame[3] = 0x02;
  FBusFrame[4] = 0x00;
  FBusFrame[5] = FrameSize;
  FBusFrame[6] = 0x00;
  FBusFrame[7] = 0x01;
  FBusFrame[8] = 0x00;
  FBusFrame[9] = 0x01;
  FBusFrame[10] = 0x02;
  FBusFrame[11] = 0x00;

  // Inserimento del numero del centro messaggi
  for (j = 0; j < sizeof(SMSC); j++) {
    FBusFrame[12 + j] = SMSC[j];
  }

  FBusFrame[24] = 0x15; //Message type
  FBusFrame[28] = MsgLen; //Lunghezza del messaggio (non compresso)

  // Inserimento del numero del ricevente
  for (j = 0; j < sizeof(RecipientNo); j++) {
    FBusFrame[j + 29] = RecipientNo[j];
  }

  FBusFrame[41] = 0xA7; // Validity period

  // Check if the Framesize is odd or even
  if (FrameSize & 0x01) {
    frameIndex = FrameSize + 5;
    FBusFrame[frameIndex] = SeqNo;
    frameIndex++;
    FBusFrame[frameIndex] = 0; // Insert to make the Frame even
    frameIndex++;
  }
  else {
    frameIndex = FrameSize + 5;
    FBusFrame[frameIndex] = SeqNo;
    frameIndex++;
  }

  // Calculate the checksum from the start of the frame to the end of the frame
  for (unsigned char i = 0; i < frameIndex+2; i += 2) {
    oddCheckSum ^= FBusFrame[i];
    evenCheckSum ^= FBusFrame[i+1];
  }
  FBusFrame[frameIndex] = oddCheckSum;
  FBusFrame[frameIndex+1] = evenCheckSum;

  // Invio dell'intero frame al Nokia 3310
  for (unsigned char j = 0; j < (frameIndex+2); j++) {
    // Debug to check in hex what we are sending
    //Serial.print(FBusFrame [j], HEX);
    //Serial.print(" ");

    Serial.write(FBusFrame [j]);
  }
}


