#define STX_CH  ('[')
#define ETX_CH  (']')
#define SZ_TEMP 16
#define SZ_RX_BUFF 64


typedef enum {
  CMD_INVALID = -1,
  CMD_NONE = 0,
  CMD_REQ  = '?',
  CMD_SET  = '!',
} enumCmd; /// command type

typedef enum {
  PER_INVALID  = -1,
  PER_NONE     = 0,
  PER_DI          /*! Digital Input   */,
  PER_DO          /*! Digital Output  */,
  PER_AI          /*! Analog Input    */,
  PER_AO          /*! Analog Output   */,
  PER_PM          /*! Parameter       */,
} enumPer; /// peripheral

typedef enum {
  PIN_INVALID  = -1,
  PIN_NONE     = 0,
  PIN_01          /*! PIN 01 */,
  PIN_02          /*! PIN 02 */,
  PIN_03          /*! PIN 03 */,
  PIN_04          /*! PIN 04 */,
  PIN_05          /*! PIN 05 */,
} enumPin; /// pin address

enum {
  D0 = 0,
  D1 ,
  D2 ,
  D3 ,
  D4 ,
  D5 ,
  D6 ,
  D7 ,
  D8 ,
  D9 ,
  D10 ,
  D11 ,
  D12 ,
  D13 ,
};

int iCounter = 0;
char ku8aRawRxMsg[SZ_RX_BUFF];
long lSzRxMsg = 0;

//--------------------------------------------------------------
long serRead( char *msg , long lSzMsg)
{
  int ii = 0;

  if ( Serial.available() > 0 )
  {
    while ( Serial.available()>0 && ii<lSzMsg )
    {
      char c = Serial.read();
      msg[ii]=c;
      ii++;
    }
  }
  return(ii);
}
//-------------------------------------------------------------
char * getNearestStx(const char *pu8aRawRxMsg, const char *pEtx)
{
  char *pStx = NULL;
  long lSzMSg = pEtx-pu8aRawRxMsg;
  char *pDummy = (char *)memchr (pu8aRawRxMsg, STX_CH, lSzMSg);
  while (pDummy!=NULL && pDummy<pEtx)
  {
    pStx = pDummy;
    pDummy = (char *)memchr (pStx+1, STX_CH, lSzMSg);
  }
  return(pStx);
}
//-------------------------------------------------------------
enumCmd getCmd (char * pu8aValidMsg, long lSzMsg)
{
  enumCmd enCmd = CMD_INVALID;
  if(pu8aValidMsg!=NULL && lSzMsg>0)
  {
    switch(pu8aValidMsg[0])
    {
    case CMD_REQ:
      enCmd = CMD_REQ;
      break;
    case CMD_SET:
      enCmd = CMD_SET;
      break;
    default:
      enCmd = CMD_INVALID;
    }
  }
  return(enCmd);
}
//-------------------------------------------------------------
enumPer getPeriph (char * pu8aValidMsg, long lSzMsg)
{
  enumPer enPeriph = PER_INVALID;
  if(pu8aValidMsg!=NULL && lSzMsg>0)
  {
    char u8aPeriph[3];
    memcpy(u8aPeriph,pu8aValidMsg+1,sizeof(u8aPeriph));
    u8aPeriph[2] = 0;

    if( strcmp(u8aPeriph,"DI")==0)
    {
      enPeriph = PER_DI;
    }
    else if( strcmp(u8aPeriph,"DO")==0)
    {
      enPeriph = PER_DO;
    }
    else if( strcmp(u8aPeriph,"AI")==0)
    {
      enPeriph = PER_AI;
    }
    else if( strcmp(u8aPeriph,"AO")==0)
    {
      enPeriph = PER_AO;
    }
    else if( strcmp(u8aPeriph,"PM")==0)
    {
      enPeriph = PER_PM;
    }
    else
    {
      enPeriph = PER_INVALID;
    }
  }
  return(enPeriph);
}
//-------------------------------------------------------------
/**
 * @brief get Payload Long value
 * @param pu8aValidMsg  msg pointer
 * @param lSzMsg        msg length
 * @param pbOk          flag to be moved to understand if conversion is succesfull
 * @return value derived
 */
long getPayloadLong (char * pu8aValidMsg, long lSzMsg, bool * pbOk)
{
  *pbOk = false;
  long lVal = -1;
  if(pu8aValidMsg!=NULL && lSzMsg>0 && lSzMsg<16)
  {
    char u8aPeriph[16];
    //Serial.print("@");
    memcpy(u8aPeriph, pu8aValidMsg+1, lSzMsg);
    u8aPeriph[lSzMsg] = 0;
    char *pLong = (char *)memchr (u8aPeriph, '.', lSzMsg);
    char * pEnd=NULL;
    long li1 = strtol(++pLong,&pEnd,10);
    //Serial.print(li1);
    if (pEnd > pLong )
    {
      lVal= li1;
      *pbOk = true;
      //Serial.print("a");
    }
  }
  //Serial.println(lVal);
  return(lVal);
}
//-------------------------------------------------------------
/**
 * @brief get Id of the sub-device to
 * @param pu8aValidMsg  msg pointer
 * @param lSzMsg        msg length
 * @param pbOk          flag to be moved to understand if conversion is succesfull
 * @return
 */
long getSubDevice (char * pu8aValidMsg, long lSzMsg, bool * bOk)
{
  long lPin = PIN_INVALID;
  *bOk = false;
  if(pu8aValidMsg!=NULL && lSzMsg>0)
  {
    char u8aPeriph[3];
    memcpy(u8aPeriph,pu8aValidMsg+3,sizeof(u8aPeriph));
    u8aPeriph[2] = 0;
    char * pEnd=NULL;
    long li1 = strtol (u8aPeriph,&pEnd,10);
    if (pEnd > u8aPeriph && li1 >= 0)
    {
      lPin = li1;
      *bOk = true;
    }
    else {
      lPin = PIN_INVALID;
    }
  }
  return(lPin);
}
//-------------------------------------------------------------
int getValidMessage (char * pu8aValidMsg, long lSzMsg)
{
  long lSzPayload = 0; // sz of valid message
  if(pu8aValidMsg!=NULL)
  {
    char * pStx = (char *) memchr (pu8aValidMsg, STX_CH, lSzMsg);
    char * pEtx = (char *) memchr (pu8aValidMsg, ETX_CH, lSzMsg);
    if (pEtx > pStx && pStx != NULL)
    {
      lSzPayload = pEtx-pStx;
      *pEtx = 0;
      memmove(pu8aValidMsg, pStx+1, lSzPayload);
      lSzPayload = strlen(pu8aValidMsg);
    }
  }
  return(lSzPayload);
}
//-------------------------------------------------------------
int extractMsg (char * pu8aValidMsg)
{
  int nChRxd;
  char
      *pStx = 0,
      *pEtx = 0;
  int lSzValidMsg = 0; // size of extracted message
  char tempBuffer[SZ_TEMP];

  nChRxd = Serial.available();
  if (nChRxd > 0)
  {
    nChRxd = serRead(tempBuffer, sizeof(tempBuffer)); // these are the new chars to add to the RX buffer;
    tempBuffer[nChRxd] = 0;
  }
  if (nChRxd > 0)
  {
    if ((lSzRxMsg + nChRxd) < SZ_RX_BUFF)
    {
      memcpy (&ku8aRawRxMsg[lSzRxMsg], tempBuffer, nChRxd);
      lSzRxMsg += nChRxd;
      ku8aRawRxMsg[lSzRxMsg] = (char)0;
    }
    else
    {   // Too many chars!!!
      memset (ku8aRawRxMsg, 0, sizeof(ku8aRawRxMsg));
      lSzRxMsg = 0;
      nChRxd = 0;
    }
  }

  if (lSzRxMsg > 0)
  { // If I have received something of new
    //--- controllo se ho ricevuto un ACK, NAK o ETX
    pEtx = (char *)memchr (ku8aRawRxMsg, ETX_CH, lSzRxMsg);
    if ( pEtx != NULL )
    {
      pStx = getNearestStx(ku8aRawRxMsg, pEtx );
      if ( pStx!=NULL && pStx < pEtx )
      { // found STX before ETX // [!dfhg]
        lSzValidMsg = (pEtx - pStx) + 1;
        //--- estraggo la parte di messaggio che mi interessa
        memcpy (pu8aValidMsg, pStx, lSzValidMsg); // included ACK, NAK or ETX
        pu8aValidMsg[lSzValidMsg]=0;
        pEtx++;
        lSzRxMsg = lSzRxMsg - (pEtx - &ku8aRawRxMsg[0]);

        if (lSzRxMsg > 0) {

          memmove(ku8aRawRxMsg, pEtx, lSzRxMsg);
        } else {
          lSzRxMsg = 0;
        }
        ku8aRawRxMsg[lSzRxMsg] = 0;
      }
    }
  }
  return lSzValidMsg;
}
//------------------------------------------------------------------------------
void processReqDigitalPin(long lPin, char * u8aMsgTx, long lSzPld)
{
  long lVal;
  bool bOk = true;
  if ( u8aMsgTx!=NULL && lSzPld>0 )
  {
    if(bOk)
    {
      switch (lPin)
      {
      case D0 :
      case D1 :
      case D2 :
      case D3 :
      case D4 :
      case D5 :
      case D6 :
      case D7 :
      case D8 :
      case D9 :
      case D10:
      case D11:
      case D12:
      case D13:

        lVal = digitalRead(lPin);
        //Serial.println(lVal);
        snprintf(u8aMsgTx, lSzPld, "%c?DI%02li.%li%c", STX_CH, lPin ,lVal, ETX_CH );
        break;
      default:
        snprintf(u8aMsgTx, lSzPld,"%c?DI%ld:%c", STX_CH, lPin , ETX_CH );
        break;
      }
      Serial.println(u8aMsgTx);
    }
  }
}
//------------------------------------------------------------------------------
void processReqAnalogPin(long lPin, char * u8aMsgTx, long lSzPld)
{
  long lVal;
  bool bOk = true;
  if ( u8aMsgTx!=NULL && lSzPld>0 )
  {
    if(bOk)
    {
      switch (lPin)
      {
      case A0 :
      case A1 :
      case A2 :
      case A3 :
      case A4 :
      case A5 :
        lVal = analogRead(lPin);
        snprintf(u8aMsgTx, lSzPld, "%c?AI%ld.%ld%c", STX_CH, lPin ,lVal, ETX_CH );
        break;
      default:
        snprintf(u8aMsgTx, lSzPld,"%c?AI%ld.%c", STX_CH, lPin , ETX_CH );
        break;
      }
      //Serial.println(u8aMsgTx);
    }
  }
}
//------------------------------------------------------------------------------
/**
 * @brief crunch messdsage to se Set Do
 * @param lPin      pin numebr to set
 * @param u8aMsgRx  ptr to mesasge to process
 * @param lSzPld  sz of message
 */
void processSetDigitalPin(long lPin, char * u8aMsgRx, long lSzPld)
{
  bool bOk = true;
  //Serial.print("D");
  if ( u8aMsgRx!=NULL && lSzPld>0 )
  {
    //Serial.print("E");
    long lVal = getPayloadLong(u8aMsgRx, lSzPld, &bOk);
    if( bOk )
    {
      //Serial.println("F");
      switch (lVal)
      {
      case LOW:
      case HIGH:
        //Serial.println(ACK_CH);
        //Serial.print(lVal+"!");
        //Serial.print(lPin);
        digitalWrite( lPin, lVal);
        break;
      default:
        Serial.print("default");
        break;
      }
    }
  }
}
//------------------------------------------------------------------------------
/**
 * @brief crunch messdsage to se Set Do
 * @param lPin      pin numebr to set
 * @param u8aMsgRx  ptr to mesasge to process
 * @param lSzPld  sz of message
 */
void processSetAnalogPin(long lPin, char * u8aMsgRx, long lSzPld)
{
  bool bOk = true;
  long lVal = -1;
  if ( u8aMsgRx!=NULL && lSzPld>0 )
  {
    lVal = getPayloadLong(u8aMsgRx, lSzPld, &bOk);
    if( bOk )
    {
      switch (lPin)
      {
      case A0 :
      case A1 :
      case A2 :
      case A3 :
      case A4 :
      case A5 :
        analogWrite( lPin, lVal);
        //Serial.println(ACK_CH);
        break;

      default:
        break;
      }
    }
  }
}
//------------------------------------------------------------------------------
/**
 * @brief manage Serial communication
 */
void crunchSerial(void)
{
  char u8aMsgRx[10] = {0,};
  char u8aMsgTx[10] = {0,};
  long lNumEl = extractMsg (u8aMsgRx);
  //Serial.print(".");
  if( lNumEl > 0 ) // [.....]
  {
    //Serial.println("A");
    long lSzPld = getValidMessage (u8aMsgRx, lNumEl);
    if ( lSzPld > 0 )
    {
      iCounter = 0;
      enumCmd enCmd = getCmd(u8aMsgRx, lSzPld);
      enumPer enPer = getPeriph(u8aMsgRx, lSzPld);
      bool bOk;
      long lPin = getSubDevice(u8aMsgRx, lSzPld, &bOk);
      //Serial.print("B");
      if( bOk && enCmd == CMD_SET )
      {
        //Serial.print("C");
        switch (enPer)
        {
        case PER_DO:
          processSetDigitalPin(lPin, u8aMsgRx, lSzPld);
          break;
        case PER_AO:
          processSetAnalogPin(lPin, u8aMsgRx, lSzPld);
          break;
        case PER_PM:
          break;
        case PER_DI:
        case PER_AI:
        default:
          // error
          break;
        }
      }
      if( enCmd == CMD_REQ )
      {
        long lSzMsg = sizeof(u8aMsgTx);
        switch (enPer)
        {
        case PER_DI:
        case PER_DO:
          processReqDigitalPin(lPin, u8aMsgTx, lSzMsg);
          break;
        case PER_AI:
        case PER_AO:
          processReqAnalogPin(lPin, u8aMsgTx, lSzMsg);
          break;
        case PER_PM:
        default:
          // error
          break;
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
void checkActivity(void)
{
  if (iCounter > 400)
  {
    for (int ii = 1; ii <= 13; ii++)
    {
      digitalWrite(ii,LOW);
    }
  }
  iCounter++;
}
//------------------------------------------------------------------------------
void setup(void)
{
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, INPUT);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}
//------------------------------------------------------------------------------
void loop(void)
{
  crunchSerial();
  checkActivity();
  delay(50);
}
//------------------------------------------------------------------------------