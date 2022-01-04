#include <Arduino.h>
#include <string.h>
#include <math.h>
long lDelayBoost = 20;

//-----------------------------
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


int New;
int K;
//----------------------------

int lSupLimit = 50;
int lInfLimit = 200;
const int lBreakRun_mm = 5; // Delta mm to break or boost

struct
{
	int  lCycles ;/// param Num cycles
	int  lHeight;/// param Height
	long lWait  ;/// param ms to wait in reverting direction
	bool bMove  ;/// param saying to move
} stParam;
enum {
	PAR_CYCLES = 1,
	PAR_HEIGHT = 2,
	PAR_MOVE   = 3,
	PAR_WAIT   = 4,
};

int lCurrCycle = 0; /// current cycle number

typedef enum {
	ST_INIT = 0,
	ST_UP,
	ST_WAIT_UP,
	ST_WAIT_DWN,
	ST_DWN,
	ST_STOP,
	ST_HOME,
	ST_BOOST_UP,
	ST_BREAK_UP,
	ST_BOOST_DWN,
	ST_BREAK_DWN,
	ST_RESET,
	ST_PAUSE,
} enumState ;

enumState enState = ST_INIT;
enumState enOldState = ST_INIT;

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
#define PIN_POTENT       A1 // potentiometer : mm*0,57703=A4 || A4*1.733 = mm
#define PIN_THERM_A      A4 // termistor A
#define PIN_THERM_B      A5 // termistor B
#define PIN_UP           13 // Enable  : DO2.1,
#define PIN_DOWN         4  // Go UP   : DO3.0,
#define PIN_ENABLE       2  // Go Down : DO4.0,


void setup(void)
{
	pinMode(3, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(9, OUTPUT);
	pinMode(10, OUTPUT);
	pinMode(13, OUTPUT);
	Serial.begin(9600);

	stParam.lCycles  = 0; // param Num cycles
	stParam.lHeight  = 0; // param Height
	stParam.lWait    = 0; // param ms to wait in reverting direction
}


//--------------------------------------------------------------
//altre funzioni
double calcTemp(int value, double R0) {
	int B = 4450;
	double V = 5 / 1023.00 * value;
	double R = (R0 * 5.00 / V) - R0;
	float esp = (float)(B) / -298.15;
	double Rinf = 47000 * pow(M_E, esp);
	double T = B / (log(R / Rinf));
	return T - 273.15;
}
//-------------------------------------------------------------
/**
 * Transmission of Potentiometer value
 * @param lVal   value to be sent on RS232
 */
void sendPotentiometer(long lVal)
{
	Serial.print("[POT:\t");
	Serial.print(lVal);
	Serial.print("]");
}
//------------------------------------------------------------------------------
void setDoUp(void)
{
	// ALL stop: DO2.0,

	// Enable  : DO2.1,
	// Go UP   : DO3.0,
	// Go Down : DO4.0,
	digitalWrite(PIN_UP  , LOW );
	digitalWrite(PIN_DOWN , HIGH);
}
//------------------------------------------------------------------------------
void setDoDown(void)
{
	digitalWrite(PIN_DOWN , LOW );
	digitalWrite(PIN_UP  , HIGH);
}
//------------------------------------------------------------------------------
void setDoStop(void)
{
	digitalWrite(PIN_DOWN , LOW );
	digitalWrite(PIN_UP  , LOW );
}
//------------------------------------------------------------------------------

void updateLimits()
{
	lSupLimit = lInfLimit + stParam.lHeight;
	lInfLimit += lBreakRun_mm; // reduce run for break / boost space
	lSupLimit -= lBreakRun_mm;
}
//------------------------------------------------------------------------------

void setState(const enumState enVal)
{
	enOldState = enState;
	enState = enVal;
}
//------------------------------------------------------------------------------
void stateReset()
{
	stParam.lCycles = 0;
	stParam.bMove = 0;
	Serial.println("RESET");
	setup();
	setState(ST_INIT);
}//-----------------------------------------------------------------------------
void statePause(void)
{
	setDoStop();
	Serial.println("Pause");
	updateLimits();
	if ( stParam.bMove == 1 )
	{
		setState(enOldState);
	}
}

//------------------------------------------------------------------------------
/**
 * @brief get Height in mm
 * @return value [mm]
 */
long getHeight()
{
	long lPotAnalog = analogRead(PIN_POTENT);
	long lPot_mm = (long)( 1.733f * (float) lPotAnalog);
	return(lPot_mm);
}
//------------------------------------------------------------------------------

void stateInit(void)
{
	long lHeight_mm = getHeight();

	Serial.print("(st_init) H_mm: "); //inutile ma fa capire che entra nello stato initilize
	Serial.println(lHeight_mm);

	if (stParam.lHeight > 0 && stParam.lCycles > 0)
	{
		setState(ST_HOME);
	}
}
//------------------------------------------------------------------------------------

void stateGoUp(void)
{
	long lHeight_mm = getHeight();
	setDoUp();
	Serial.print("(st_up) H_mm: ");
	Serial.println(lHeight_mm);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm >= lSupLimit)
	{
		setState(ST_BREAK_UP);
	}

}
//------------------------------------------------------------------------------------

void stateWaitUp(void)
{
	setDoStop();
	delay(stParam.lWait);

	Serial.print("(st_wait_up) Reverting Direction (DOWN -> UP) ");
	Serial.print("Completed cycles: ");
	Serial.println(lCurrCycle);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lCurrCycle >= stParam.lCycles)
	{
		setState(ST_STOP);
	}
	else
	{
		setState(ST_BOOST_UP);
	}
}
//------------------------------------------------------------------------------------

void stateWaitDown()
{
	setDoStop();
	delay(stParam.lWait);
	Serial.println("Reverting Direction (UP -> DOWN)");
	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	} else {
		setState(ST_BOOST_DWN);
	}
}
//------------------------------------------------------------------------------------

void stateGoDown()
{
	setDoDown();
	long lHeight_mm = getHeight();

	Serial.print("(st_down) H_mm: ");
	Serial.println(lHeight_mm);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm <= lInfLimit)
	{
		lCurrCycle++;
		setState(ST_BREAK_DWN);
	}

}
//------------------------------------------------------------------------------------

void stateEmergency(void)
{
	setDoStop();
	Serial.println("Emergency!");
	delay(2000);
}
//------------------------------------------------------------------------------------

void stateStop()
{
	setDoStop();
	Serial.println("Stop: End of test");
	delay(2000);
	stateReset();
}

//------------------------------------------------------------------------------
void stateHoming()
{
	setDoDown();
	long lheight0 = getHeight();
	delay(1000);
	long lheight1 = getHeight();

	Serial.print("(st_homing) H_mm: ");
	Serial.println(lheight1);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lheight1 - lheight0 < 5)
	{
		setDoStop();
		lCurrCycle = 0;
		lInfLimit = lheight0 ;
		updateLimits(); // reduce run for break / boost space
		setState(ST_WAIT_UP);
	}
}
//------------------------------------------------------------------------------------

void stateBoostUp()
{
	setDoUp();
	long lHeight_mm = getHeight();

	Serial.print("(st_boostUp) H_mm: ");
	Serial.println(lHeight_mm);


	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm >= lInfLimit)
	{
		setState(ST_UP);
	}
}
//------------------------------------------------------------------------------------

void stateBoostDown()
{
	setDoDown();
	long lHeight_mm = getHeight();

	Serial.print("(st_boostDown) H_mm: ");
	Serial.println(lHeight_mm);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm <= lSupLimit)
	{
		setState(ST_DWN);
	}
}
//------------------------------------------------------------------------------------

void stateBreakUp()
{
	long lHeight_mm = getHeight();
	setDoUp(); // replace with break!
	Serial.print("(st_breakUp) H_mm: ");
	Serial.println(lHeight_mm);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm >= lSupLimit)
	{
		setState(ST_WAIT_DWN);
	}
}
//------------------------------------------------------------------------------------

void stateBreakDown()
{
	long lHeight_mm = getHeight();
	setDoDown(); // replace with break!
	Serial.print("(st_breakDown) H_mm: ");
	Serial.println(lHeight_mm);

	if ( stParam.bMove == 0 )
	{
		setState(ST_PAUSE);
	}else if (lHeight_mm <= lInfLimit)
	{
		setDoStop();
		setState(ST_WAIT_UP);
	}
}
//---------------------------------------------------------------------
void stateMachine()
{
	switch (enState)
	{
	case ST_INIT:
		stateInit();
		break;
	case ST_UP:
		stateGoUp();
		break;
	case ST_WAIT_UP:
		stateWaitUp();
		break;
	case ST_WAIT_DWN:
		stateWaitDown();
		break;
	case ST_DWN:
		stateGoDown();
		break;
	case ST_STOP:
		stateStop();
		break;
	case ST_HOME:
		stateHoming();
		break;
	case ST_BOOST_UP:
		stateBoostUp();
		break;
	case ST_BOOST_DWN:
		stateBoostDown();
		break;
	case ST_BREAK_UP:
		stateBreakUp();
		break;
	case ST_BREAK_DWN:
		stateBreakDown();
		break;
	case ST_RESET:
		stateReset();
		break;
	case ST_PAUSE:
		statePause();
		break;
	default:
		stateEmergency();
		break;
	}
}


//-------------------------------------------------------------

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
#define STX_CH  ('[')
#define ETX_CH  (']')
#define ACK_CH  ('£')
#define NAK_CH  (0x15)
#define SZ_TEMP 16
#define SZ_RX_BUFF 64

char ku8aRawRxMsg[SZ_RX_BUFF];
long lSzRxMsg = 0;

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
			{	// found STX before ETX // [!dfhg]
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


//------------------------------------------------------------------------------------

void callback()
{
	digitalWrite(10, digitalRead(10) ^ 1);
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

void processReqParam(long lParam, char * u8aMsgTx, long lSzPld)
{
	long lVal;
	bool bOk = true;
	if ( u8aMsgTx!=NULL && lSzPld>0 )
	{
		switch (lParam)
		{
		case PAR_CYCLES://[!PM01.yyyyyyyy]
			lVal = stParam.lCycles;
			break;
		case PAR_HEIGHT://[!PM02.yyyyyyyy]
			lVal = stParam.lHeight;
			break;
		case PAR_MOVE://[!PM03.yyyyyyyy]
			lVal = stParam.bMove;
			break;
		default:
			bOk = false;
			break;
		}

		if ( bOk ){
			snprintf(u8aMsgTx, lSzPld, "%c?PM%ld.%ld%c", STX_CH, lParam ,lVal, ETX_CH );
		} else {
			snprintf(u8aMsgTx, lSzPld,"%c?PM%ld.%c", STX_CH, lParam , ETX_CH );
		}
		//Serial.println(u8aMsgTx);

	}
}

//------------------------------------------------------------------------------
/**
 * @brief crunch messdsage to se Set Do
 * @param lPin      pin numebr to set
 * @param u8aMsgRx	ptr to mesasge to process
 * @param lSzPld	sz of message
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
 * @param u8aMsgRx	ptr to mesasge to process
 * @param lSzPld	sz of message
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

void crunchRxDo(long lPin, char * u8aMsgRx, long lSzPld)
{
	bool bOk = true;
	long lVal = -1;
	if ( u8aMsgRx!=NULL && lSzPld>0 )
	{

		lVal = getPayloadLong(u8aMsgRx, lSzPld, &bOk);
		if(bOk)
		{
			switch (lPin)
			{
			case 1:
				stParam.lCycles = lVal;
				break;
			case 2:
				stParam.lHeight = lVal;
				break;

			default:
				break;
			}
		}
	}
}
//------------------------------------------------------------------------------
void processSetParam(long lParamId, char * u8aMsgRx, long lSzPld)
{
	bool bOk = true;
	long lVal = -1;
	if ( u8aMsgRx!=NULL && lSzPld>0 )
	{
		lVal = getPayloadLong(u8aMsgRx, lSzPld, &bOk);
		if(bOk)
		{
			switch (lParamId)
			{
			case PAR_CYCLES://[!PM01.yyyyyyyy]
				stParam.lCycles = lVal;
				break;
			case PAR_HEIGHT://[!PM02.yyyyyyyy]
				stParam.lHeight = lVal;
				break;
			case PAR_MOVE://[!PM03.yyyyyyyy]
				stParam.bMove = (lVal!=0);
				break;
			default:
				break;
			}
			//Serial.println(ACK_CH);

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
				case PER_PM://[!PMxx.yyyyyyyy]
					processSetParam(lPin, u8aMsgRx, lSzPld);
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
					processReqParam(lPin, u8aMsgTx, lSzMsg);
					break;
				default:
					// error
					break;
				}
			}
		}
	}
}
//------------------------------------------------------------------------------
/**
 * @brief loop
 */
void loop(void)
{
	crunchSerial();
	//sateMachine();
	delay(50);
}






