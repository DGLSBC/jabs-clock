/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#include "svgData.h" //
#include "global.h"  // for ?
#include "draw.h"    // for ?

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
/*
scour --enable-viewboxing --create-groups --shorten-ids --enable-id-stripping --enable-comment-stripping --disable-embed-rasters --remove-metadata --strip-xml-prolog -p 9 < clock-face.svg > clock-face-new.svg
*/
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const char g_svgDefFace[] =
"<svg viewBox=\"0 0 100 100\">\n"
"  <path d=\"m47.791164 22.941767a18.674698 12.700803 0 1 1 -37.349395 0 18.674698 12.700803 0 1 1 37.349395 0z\" fill-opacity=\".50181819\" transform=\"matrix(1.259313,-1.733296,2.548562,1.85164,-45.13394,57.98624)\" stroke=\"#333\" stroke-linecap=\"square\" stroke-width=\".1924613\" fill=\"none\"/>\n"
//"  <path d=\"m47.791164 22.941767a18.674698 12.700803 0 1 1 -37.349395 0 18.674698 12.700803 0 1 1 37.349395 0z\" fill-opacity=\".50181819\" transform=\"matrix(1.259313,-1.733296,2.548562,1.85164,-45.13394,57.98624)\" stroke=\"#CCC\" stroke-linecap=\"square\" stroke-width=\".1924613\" fill=\"none\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefMarks[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <g stroke=\"#000\">\n"
//" <g stroke=\"#fff\">\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" fill=\"none\">\n"
"   <g stroke-width=\".29999983\">\n"
"    <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"    <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"    <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"    <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"    <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"    <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"    <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"    <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"   </g>\n"
"   <g stroke-width=\".5\">\n"
"    <path d=\"m50.003907 10.248353v4.417671\"/>\n"
"    <path d=\"m89.759991 50.00446h-4.417671\"/>\n"
"    <path d=\"m50.003893 89.760545v-4.417671\"/>\n"
"    <path d=\"m10.247805 50.004437h4.417671\"/>\n"
"   </g>\n"
"  </g>\n"
" </g>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefMarks24[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <g stroke=\"#000\">\n"
//" <g stroke=\"#fff\">\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" fill=\"none\">\n"
"   <g stroke-width=\".29999983\">\n"
"    <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"    <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"    <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"    <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"    <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"    <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"    <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"    <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"   </g>\n"
"   <g stroke-width=\".5\">\n"
"    <path d=\"m50.003907 10.248353v4.417671\"/>\n"
"    <path d=\"m89.759991 50.00446h-4.417671\"/>\n"
"    <path d=\"m50.003893 89.760545v-4.417671\"/>\n"
"    <path d=\"m10.247805 50.004437h4.417671\"/>\n"
"   </g>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"   <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" transform=\"matrix(.965926 .258819 -.258819 .965926 14.64493 -11.23824)\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"   <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"   <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"   <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"   <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" transform=\"matrix(.965926 .258819 -.258819 .965926 14.64493 -11.23824)\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562162 30.048328l-2.013443 1.162462\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m69.956112 84.558802l-1.162462-2.013443\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445638 69.952712l2.013443-1.162462\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"   <path d=\"m30.051688 15.442238l1.162462 2.013443\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" transform=\"matrix(.965926 -.258819 .258819 .965926 -11.23725 14.64569)\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"  </g>\n"
"  <g stroke-linejoin=\"round\" transform=\"matrix(.965926 -.258819 .258819 .965926 -11.23725 14.64569)\" stroke-linecap=\"round\" stroke-width=\".29999983\" fill=\"none\">\n"
"   <path d=\"m69.95611 15.442295l-1.162462 2.013443\"/>\n"
"   <path d=\"m84.562165 69.95275l-2.013443-1.162462\"/>\n"
"   <path d=\"m30.05169 84.558765l1.162462-2.013443\"/>\n"
"   <path d=\"m15.445655 30.04829l2.013443 1.162462\"/>\n"
"  </g>\n"
" </g>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefHour[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m-0.0062619239 0h19.9292739\" stroke=\"#000\" stroke-linecap=\"round\" stroke-width=\"2\" fill=\"none\"/> \n"
//" <path stroke-linejoin=\"round\" d=\"m-0.0062619239 0h19.9292739\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-width=\"2\" fill=\"none\"/> \n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefHourShadow[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m-0.0062619239 0h19.9292739\" stroke=\"#000\" stroke-linecap=\"round\" stroke-width=\"2\" stroke-opacity=\"0.125\" fill-opacity=\".75\" fill=\"#000\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefMinute[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m0.000433149 0h25.7475999\" stroke=\"#000\" stroke-linecap=\"round\" fill=\"none\"/>\n"
//" <path stroke-linejoin=\"round\" d=\"m0.000433149 0h25.7475999\" stroke=\"#fff\" stroke-linecap=\"round\" fill=\"none\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefMinuteShadow[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m0.000433149 0h25.7475999\" stroke=\"#000\" stroke-linecap=\"round\" stroke-opacity=\"0.125\" fill-opacity=\".75\" fill=\"#000\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefSecond[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m-0.00186982 0h31.8673928\" stroke=\"#000\" stroke-linecap=\"round\" stroke-width=\".5\" fill=\"none\"/>\n"
//" <path stroke-linejoin=\"round\" d=\"m-0.00186982 0h31.8673928\" stroke=\"#fff\" stroke-linecap=\"round\" stroke-width=\".5\" fill=\"none\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefSecondShadow[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path stroke-linejoin=\"round\" d=\"m-0.00186982 0h31.8673928\" stroke=\"#000\" stroke-linecap=\"round\" stroke-width=\".5\" stroke-opacity=\"0.125\" fill-opacity=\".75\" fill=\"#000\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefGlass[] =
"<svg viewBox=\"0 0 100 100\">\n"
" <path d=\"m49.03125 9.9375c-21.437648 0.543731-38.727315 17.996403-39 39.5-0.002181 0.172029 0 0.327457 0 0.5s-0.002181 0.359221 0 0.53125c0.065764 5.186074 1.137442 10.123536 3 14.65625 8.656995-32.861415 34.335538-46.183854 53.6875-51.5625-4.784255-2.199475-10.051212-3.48363-15.625-3.625h-2.0625zm40.9375 37.1875c-6.361623 28.277465-28.301361 38.614963-47.59375 42.09375 2.321552 0.451756 4.71052 0.718986 7.15625 0.75h1.03125c21.503601-0.272686 38.956269-17.562356 39.5-39 0.0087-0.343002 0-0.686163 0-1.03125s0.0087-0.688248 0-1.03125c-0.015224-0.600254-0.052325-1.187821-0.09375-1.78125z\" fill-opacity=\".24705882\" fill=\"#dde5f2\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefFrame[] =
"<svg viewBox=\"0 0 100 100\">\n"
"  <circle cx=\"50\" cy=\"50\" r=\"2\" stroke=\"#000\" stroke-width=\".8\" fill=\"#000\"/>\n"
//"  <circle cx=\"50\" cy=\"50\" r=\"2\" stroke=\"#fff\" stroke-width=\".8\" fill=\"#fff\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
static const char g_svgDefMask[] =
"<svg viewBox=\"0 0 100 100\">\n"
"  <path d=\"m47.791164 22.941767a18.674698 12.700803 0 1 1 -37.349395 0 18.674698 12.700803 0 1 1 37.349395 0z\" fill-opacity=\"1\" transform=\"matrix(1.259313,-1.733296,2.548562,1.85164,-45.13394,57.98624)\" stroke=\"#577336\" stroke-linecap=\"square\" stroke-width=\".1924613\" fill=\"#577336\"/>\n"
"</svg>\n";

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//  NOTE: must subtract 1 from length or loading fails

static int   g_svgDefFaceLen   = vectsz(g_svgDefFace)         - 1;
static int   g_svgDef12MrksLen = vectsz(g_svgDefMarks)        - 1;
static int   g_svgDef24MrksLen = vectsz(g_svgDefMarks24)      - 1;
static int   g_svgDefHorHndLen = vectsz(g_svgDefHour)         - 1;
static int   g_svgDefMinHndLen = vectsz(g_svgDefMinute)       - 1;
static int   g_svgDefSecHndLen = vectsz(g_svgDefSecond)       - 1;
static int   g_svgDefHorShdLen = vectsz(g_svgDefHourShadow)   - 1;
static int   g_svgDefMinShdLen = vectsz(g_svgDefMinuteShadow) - 1;
static int   g_svgDefSecShdLen = vectsz(g_svgDefSecondShadow) - 1;
static int   g_svgDefGlassLen  = vectsz(g_svgDefGlass)        - 1;
static int   g_svgDefFrameLen  = vectsz(g_svgDefFrame)        - 1;
static int   g_svgDefMaskLen   = vectsz(g_svgDefMask)         - 1;

const  char* g_svgDefData[] =
{
	g_svgDefFace,
	g_svgDefMarks,      g_svgDefMarks24,
	g_svgDefHour,       g_svgDefMinute,       g_svgDefSecond,
	g_svgDefHourShadow, g_svgDefMinuteShadow, g_svgDefSecondShadow,
	g_svgDefGlass,
	g_svgDefFrame,
//	g_svgDefMask,
	NULL
};

const  int   g_svgDefDLen[] =
{
	g_svgDefFaceLen,
	g_svgDef12MrksLen,  g_svgDef24MrksLen,
	g_svgDefHorHndLen,  g_svgDefMinHndLen,    g_svgDefSecHndLen,
	g_svgDefHorShdLen,  g_svgDefMinShdLen,    g_svgDefSecShdLen,
	g_svgDefGlassLen,
	g_svgDefFrameLen,
//	g_svgDefMaskLen,
	0
};

const  int   g_svgDefDTyp[] =
{
	draw::CLOCK_FACE,
	draw::CLOCK_MARKS,            draw::CLOCK_MARKS_24H,
	draw::CLOCK_HOUR_HAND,        draw::CLOCK_MINUTE_HAND,        draw::CLOCK_SECOND_HAND,
	draw::CLOCK_HOUR_HAND_SHADOW, draw::CLOCK_MINUTE_HAND_SHADOW, draw::CLOCK_SECOND_HAND_SHADOW,
	draw::CLOCK_GLASS,
	draw::CLOCK_FRAME,
//	draw::CLOCK_MASK,
	-1
};

