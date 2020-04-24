#define AMPM_Fmt   // When 12 hour Format required else comment it out by // in the front

const int utc_hr_difference = 5;  // IST is 5 Hr 30min Ahead. Use -ve no. if behind utc
const int utc_min_difference = 30;

// Colour codes for TFT
#define BLACK       0x0000      /*   0,   0,   0 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define GREY        0x7BEF      /* 128, 128, 128 */
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define BLUE        0x001F      /*   0,   0, 255 */
#define LBLUE       0xC6FE
#define NAVY        0x000F      /*   0,   0, 128 */
#define RED         0xF800      /* 255,   0,   0 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define WHITE       0xFFFF      /* 255, 255, 255 */
#define PINK        0xF81F
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define GREEN       0x07E0
#define DARKGREEN   0x03E0      /*   0, 128,   0 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define CYAN        0x07FF
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define MAGENTA     0xF81F

#define LIME     0x07FF   
#define AQUA     0x04FF   
#define LTPINK   0xFDDF   
#define LTGREY   0xE71C    
#define PINKY    0xF8FF    



// Display Themes and Color selection
// Select any one theme by removing // , unselect by // in front
// Colurs can be substituted from table above

//#define LightTheme    
#define DarkTheme 

// Color choices for LightTheme
#ifdef LightTheme
#define BackColor LBLUE
#define TimeColor NAVY
#define DayColor DARKGREEN
#define DateColor OLIVE
#define InTempColor DARKGREEN
#define OutTempColor RED
#define PressColor YELLOW
#define HumColor WHITE
#define PartitionLineColor GREY
#endif

// Color choices for DarkTheme

#ifdef DarkTheme
#define BackColor NAVY
#define TimeColor LIME
#define DayColor LTPINK
#define DateColor LTPINK
#define InTempColor YELLOW
#define OutTempColor RED
#define PressColor LTGREY
#define HumColor PINKY
#define PartitionLineColor GREY
#endif
