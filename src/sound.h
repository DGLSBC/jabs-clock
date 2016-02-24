/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __sound_h__
#define __sound_h__

namespace snd
{

void init();
void dnit();

void play_alarm(bool newPlay);
void play_chime(bool newMinute, bool newHour);

} // namespace snd

#endif // __sound_h__

