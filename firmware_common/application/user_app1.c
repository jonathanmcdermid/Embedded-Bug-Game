/*********************************************************************************************************************
FILE user_app1.c
**********************************************************************************************************************/

#include "configuration.h"


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static u8 u8vtouch;
static u8 score=100;

PixelBlockType player = {
  .u16RowStart = 28,
  .u16ColumnStart = 0,
  .u16RowSize = 17,
  .u16ColumnSize = 25
};

PixelBlockType playerhitbox = {
  .u16RowStart = 28,
  .u16ColumnStart = 0,
  .u16RowSize = 19,
  .u16ColumnSize = 25
};
    
PixelBlockType bullethitbox = {
  .u16RowStart = 0,
  .u16ColumnStart = 0,
  .u16RowSize = 5,
  .u16ColumnSize = 6
};

PixelBlockType enemyhitbox = {
  .u16RowStart = 28,
  .u16ColumnStart = 126,
  .u16RowSize = 9,
  .u16ColumnSize = 9
};

const u8 ShipPrint[17][LCD_IMAGE_COL_BYTES_25PX] = {
{0xF6, 0x0F, 0x00, 0x00},
{0xFF, 0x0F, 0x00, 0x00},
{0xF7, 0xFF, 0x3F, 0x00},
{0xFF, 0xFF, 0x3F, 0x00},
{0xF6, 0x87, 0x2A, 0x00},
{0xE0, 0x9D, 0x3F, 0x00},
{0xF0, 0x3B, 0x00, 0x00},
{0xF8, 0xF7, 0xD5, 0x01},
{0xF8, 0xEF, 0xFF, 0x01},
{0xF8, 0xF7, 0xD5, 0x01},
{0xF0, 0x3B, 0x00, 0x00},
{0xC0, 0x9D, 0x3F, 0x00},
{0xF6, 0x87, 0x2A, 0x00},
{0xFF, 0xFF, 0x3F, 0x00},
{0xF7, 0xFF, 0x3F, 0x00},
{0xFF, 0x0F, 0x00, 0x00},
{0xF6, 0x0F, 0x00, 0x00},
};

const u8 BlackShipPrint[18][LCD_IMAGE_COL_BYTES_25PX] = {
{0x09, 0xFC, 0xFF, 0x01},
{0x00, 0xFC, 0xFF, 0x01},
{0x08, 0x00, 0x80, 0x01},
{0x00, 0x00, 0x80, 0x01},
{0x09, 0xFC, 0xAA, 0x01},
{0x1F, 0xE1, 0x80, 0x01},
{0x07, 0xC2, 0xFF, 0x01},
{0x03, 0x04, 0x2A, 0x00},
{0x03, 0x08, 0x00, 0x00},
{0x03, 0x04, 0x2A, 0x00},
{0x07, 0xC2, 0xFF, 0x01},
{0x1F, 0xE1, 0x80, 0x01},
{0x09, 0xFC, 0xAA, 0x01},
{0x00, 0x00, 0x80, 0x01},
{0x08, 0x00, 0x80, 0x01},
{0x00, 0xFC, 0xFF, 0x01},
{0x09, 0xFC, 0xFF, 0x01},
};

const u8 BulletPrint[5][LCD_IMAGE_ARROW_COL_BYTES] = {
{0x06},
{0x0C},
{0x3F},
{0x0C},
{0x06}
};

const u8 EnemyPrint[9][LCD_BIG_FONT_COLUMN_BYTES] = {
{0x0E, 0x00},
{0x11, 0x00},
{0xF8, 0x03},
{0xEE, 0x00},
{0xD8, 0x03},
{0xEE, 0x00},
{0xF8, 0x03},
{0x11, 0x00},
{0x0E, 0x00}
};

const u8 ExplodePrint[10][LCD_BIG_FONT_COLUMN_BYTES] = {
{0x4B, 0x03},
{0x87, 0x03},
{0xCE, 0x01},
{0xFD, 0x02},
{0x78, 0x00},
{0x78, 0x00},
{0xFD, 0x02},
{0xCE, 0x01},
{0x87, 0x03},
{0x4B, 0x03}
};

static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */

void UserApp1Initialize(void)
{
  LcdClearScreen();
  CapTouchOn();
  LcdLoadBitmap(&ShipPrint[0][0], &player);
  if( 1 )
  {
    UserApp1_StateMachine = UserApp1SM_Idle;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    UserApp1_StateMachine = UserApp1SM_Error;
  }

} /* end UserApp1Initialize() */

  
void UserApp1RunActiveState(void)
{
  static u8 u8cooldown=50;
  static u8 u8ticker=0;
  static u8 u8bulletticker=0;
  static u8 u8enemyticker=0;
  static bool activebullet=FALSE;
  static bool deadenemy=TRUE;
  bool drift=FALSE;
  
  if(deadenemy)
  {
    EnemyMake();
  }
  
  if(u8enemyticker>score)
  {
    u8enemyticker=0;
    DrawEnemy();
    deadenemy=CollisionCheck();
  }
  
  if(WasButtonPressed(PB_00_BUTTON1))
  {
    LcdClearPixels(&bullethitbox);
    bullethitbox.u16ColumnStart=player.u16ColumnSize;
    bullethitbox.u16RowStart=player.u16RowStart+(player.u16RowSize/2)-2;
    activebullet=TRUE;
    ButtonAcknowledge(PB_00_BUTTON1);
  }
  
  if(activebullet&&u8bulletticker>4)
  {
    u8bulletticker=0;
    DrawBullet(&activebullet);
  }
  
  if(u8ticker>u8cooldown)
  {
    u8vtouch=CaptouchCurrentVSlidePosition();
    
    if(abs(127-u8vtouch)>100)
    {
      u8cooldown=15;
      drift=TRUE;
    }
    else if(abs(127-u8vtouch)>70)
    {
      u8cooldown=30;
      drift=TRUE;
    }
    else if(abs(127-u8vtouch)>40)
    {
      u8cooldown=50;
      drift=TRUE;
    }
    else 
    {
      u8cooldown=70;
      drift=FALSE;
    }
  }
  if(u8vtouch=='\0')
  {
    u8vtouch=127;
  }
  if(u8ticker>u8cooldown||drift)
  {
    u8ticker=0;
    DrawShip(u8vtouch);
  }
  u8ticker++;
  u8enemyticker++;
  u8bulletticker++;
  UserApp1_StateMachine();
} /* end UserApp1RunActiveState */

bool CollisionCheck(void)
{
  for(u8 i=0;i<bullethitbox.u16ColumnSize;i++)
  {
    for(u8 j=0;j<bullethitbox.u16RowSize;j++)
    {
      for(u8 k=0;k<enemyhitbox.u16ColumnSize;k++)
      {
        for(u8 l=0;l<enemyhitbox.u16RowSize;l++)
        {
          if(bullethitbox.u16ColumnStart+i==enemyhitbox.u16ColumnStart+k && bullethitbox.u16RowStart+j==enemyhitbox.u16RowStart+l)
          {
           score=score-3;
           return TRUE;
          }
        }
      }
    }
  }
  return FALSE;
}

void EnemyMake(void)
{
  enemyhitbox.u16RowStart = rand() % 55;
  enemyhitbox.u16ColumnStart = 126;
}

void DrawEnemy(void)
{
  LcdClearPixels(&enemyhitbox);
  if(enemyhitbox.u16ColumnStart>0)
  {
    enemyhitbox.u16ColumnStart--;
  }
  else
  {     
    EnemyMake(); //change to lose screen
  }
  LcdLoadBitmap(&EnemyPrint[0][0], &enemyhitbox);
}

bool DrawBullet(bool* ongoing)
{
  if(bullethitbox.u16ColumnStart<127)
  {
    LcdClearPixels(&bullethitbox);
    bullethitbox.u16ColumnStart++;
    LcdLoadBitmap(&BulletPrint[0][0], &bullethitbox);
    return TRUE;
  }
  else
  {
    LcdClearPixels(&bullethitbox);
    return FALSE;
  }
}

void DrawShip(u8 slider)
{
  if(slider>157 & player.u16RowStart<47)
  {
    LcdClearPixels(&playerhitbox);
    playerhitbox.u16RowStart++;
    player.u16RowStart++;
    LcdLoadBitmap(&ShipPrint[0][0], &player);
  }
  else if(slider<97 & player.u16RowStart>0)
  {
    LcdClearPixels(&playerhitbox);
    playerhitbox.u16RowStart--;
    player.u16RowStart--;
    LcdLoadBitmap(&ShipPrint[0][0], &player);
  }
}

static void UserApp1SM_Idle(void)
{
  

} /* end UserApp1SM_Idle() */
    

/* Handle an error */
static void UserApp1SM_Error(void)          
{
  
} /* end UserApp1SM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
