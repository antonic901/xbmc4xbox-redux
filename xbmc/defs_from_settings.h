/* This enums are coming from removed GUISettings.h We should move them to appropriate place once we get XBMC compiling */

// Render Methods
#define RENDER_LQ_RGB_SHADER   0
#define RENDER_OVERLAYS      1
#define RENDER_HQ_RGB_SHADER   2
#define RENDER_HQ_RGB_SHADERV2   3

// Subtitle colours
#define SUBTITLE_COLOR_START  0
#define SUBTITLE_COLOR_END    7

// CDDA ripper defines
#define CDDARIP_ENCODER_LAME     0
#define CDDARIP_ENCODER_VORBIS   1
#define CDDARIP_ENCODER_WAV      2
#define CDDARIP_ENCODER_FLAC     3

#define CDDARIP_QUALITY_CBR      0
#define CDDARIP_QUALITY_MEDIUM   1
#define CDDARIP_QUALITY_STANDARD 2
#define CDDARIP_QUALITY_EXTREME  3

#define AUDIO_ANALOG      0
#define AUDIO_DIGITAL      1

#define VIDEO_NORMAL 0
#define VIDEO_LETTERBOX 1
#define VIDEO_WIDESCREEN 2

// LCD settings
#define LCD_TYPE_NONE        0
#define LCD_TYPE_LCD_HD44780 1
#define LCD_TYPE_LCD_KS0073  2
#define LCD_TYPE_VFD         3

#define MODCHIP_SMARTXX   0
#define MODCHIP_XENIUM    1
#define MODCHIP_XECUTER3  2

// LED settings
#define LED_COLOUR_NO_CHANGE 0
#define LED_COLOUR_GREEN   1
#define LED_COLOUR_ORANGE   2
#define LED_COLOUR_RED    3
#define LED_COLOUR_CYCLE   4
#define LED_COLOUR_OFF    5

#define FRAME_RATE_LEAVE_AS_IS  0
#define FRAME_RATE_CONVERT      1
#define FRAME_RATE_USE_PAL60    2

#define LED_PLAYBACK_OFF     0
#define LED_PLAYBACK_VIDEO    1
#define LED_PLAYBACK_MUSIC    2
#define LED_PLAYBACK_VIDEO_MUSIC 3

#define SPIN_DOWN_NONE  0
#define SPIN_DOWN_MUSIC  1
#define SPIN_DOWN_VIDEO  2
#define SPIN_DOWN_BOTH  3

#define AAM_QUIET 1
#define AAM_FAST  0

#define APM_HIPOWER 0
#define APM_LOPOWER 1
#define APM_HIPOWER_STANDBY 2
#define APM_LOPOWER_STANDBY 3

#define NETWORK_DASH   0
#define NETWORK_DHCP   1
#define NETWORK_STATIC  2

#define SETTINGS_TYPE_BOOL   1
#define SETTINGS_TYPE_FLOAT   2
#define SETTINGS_TYPE_INT    3
#define SETTINGS_TYPE_STRING  4
#define SETTINGS_TYPE_HEX    5
#define SETTINGS_TYPE_SEPARATOR 6
#define SETTINGS_TYPE_PATH      7
#define SETTINGS_TYPE_ADDON     8

#define CHECKMARK_CONTROL           1
#define SPIN_CONTROL_FLOAT          2
#define SPIN_CONTROL_INT            3
#define SPIN_CONTROL_INT_PLUS       4
#define SPIN_CONTROL_TEXT           5
#define EDIT_CONTROL_INPUT          6
#define EDIT_CONTROL_HIDDEN_INPUT   7
#define EDIT_CONTROL_NUMBER_INPUT   8
#define EDIT_CONTROL_IP_INPUT       9
#define EDIT_CONTROL_MD5_INPUT     10
#define BUTTON_CONTROL_STANDARD    11
#define BUTTON_CONTROL_MISC_INPUT  12
#define BUTTON_CONTROL_PATH_INPUT  13
#define SEPARATOR_CONTROL          14

#define RESUME_NO  0
#define RESUME_YES 1
#define RESUME_ASK 2

#define REPLAY_GAIN_NONE 0
#define REPLAY_GAIN_ALBUM 1
#define REPLAY_GAIN_TRACK 2

// Player types
#define PLAYER_MPLAYER    0
#define PLAYER_DVDPLAYER  1
#define PLAYER_PAPLAYER   2

// enum RenderMethods
// {
//   RENDER_LQ_RGB_SHADER  = 0,
//   RENDER_OVERLAYS,
//   RENDER_HQ_RGB_SHADER,
//   RENDER_HQ_RGB_SHADERV2
// };

// enum CddaRipperEncoders
// {
//   CDDARIP_ENCODER_LAME  = 0,
//   CDDARIP_ENCODER_VORBIS,
//   CDDARIP_ENCODER_WAV,
//   CDDARIP_ENCODER_FLAC
// };

// enum AudioOutputs
// {
//   AUDIO_ANALOG  = 0,
//   AUDIO_DIGITAL
// };

// enum VideoAspecRatios
// {
//   VIDEO_NORMAL  = 0,
//   VIDEO_LETTERBOX,
//   VIDEO_WIDESCREEN
// };

// enum FrameRate
// {
//   FRAME_RATE_LEAVE_AS_IS  = 0,
//   FRAME_RATE_CONVERT,
//   FRAME_RATE_USE_PAL60
// };

// enum LcdTypes{
//   LCD_TYPE_NONE        = 0,
//   LCD_TYPE_LCD_HD44780,
//   LCD_TYPE_LCD_KS0073,
//   LCD_TYPE_VFD
// };

// enum Modchips
// {
//   MODCHIP_SMARTXX = 0,
//   MODCHIP_XENIUM,
//   MODCHIP_XECUTER3
// };

// enum NetworkConfigurations
// {
//   NETWORK_DASH  = 0,
//   NETWORK_DHCP,
//   NETWORK_STATIC
// };

// enum PlayerTypes
// {
//   PLAYER_MPLAYER  = 0,
//   PLAYER_DVDPLAYER,
//   PLAYER_PAPLAYER
// };
