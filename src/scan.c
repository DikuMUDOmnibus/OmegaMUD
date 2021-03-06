/*$Id: scan.c,v 1.6 2005/03/11 17:47:55 ahsile Exp $*/

#if defined( macintosh )
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

extern char* target_name;

/*
 * Nagilum's scan function and related constants.
 * (contributed by VampLestat 17-Feb-94)
 * (modified by Mikko Kilpikoski 12-Jun-94)
 */

char * const dist_name [] =
{
  "right here",
  "close by", "not far off", "a brief walk away",
  "rather far off", "in the disatance", "almost out of sight"
};

char * const dir_desc [] =
{
  "to the north", "to the east", "to the south", "to the west",
  "upwards", "downwards"
};

void scan( CHAR_DATA *ch, int door )
{
  char buf[MAX_STRING_LENGTH];
  int distance, visibility;
  bool found;
  ROOM_INDEX_DATA *was_in_room;

  visibility = 6;
  if( !IS_SET( ch->act, PLR_HOLYLIGHT ) )
    {
      switch( weather_info.sunlight )
	{
	case SUN_SET:   visibility = 4; break;
	case SUN_DARK:  visibility = 2; break;
	case SUN_RISE:  visibility = 4; break;
	case SUN_LIGHT: visibility = 6; break;
	}
      switch( weather_info.sky )
	{
	case SKY_CLOUDLESS: break;
	case SKY_CLOUDY:    visibility -= 1; break;
	case SKY_RAINING:   visibility -= 2; break;
	case SKY_LIGHTNING: visibility -= 3; break;
	}
    }

  was_in_room = ch->in_room;
  found = FALSE;
  for( distance = 1; distance <= 6; distance++ )
    {
      EXIT_DATA *pexit;
      CHAR_DATA *list;
      CHAR_DATA *rch;

      if( ( pexit = ch->in_room->exit[door] ) != NULL
	 && pexit->to_room != NULL
	 && pexit->to_room != was_in_room )
	{
	  /* If the door is closed, stop looking... */
	  if( IS_SET( pexit->exit_info, EX_CLOSED ) )
	    {
	      char door_name[80];

	      one_argument( pexit->keyword, door_name );
	      if( door_name[0] == '\0' )
		strcat( door_name, "door" );

	      sprintf( buf, "A closed %s %s %s.\n\r",
		      door_name, dist_name[distance-1], dir_desc[door] );
	      send_to_char(AT_WHITE, buf, ch );

	      found = TRUE;
	      break;
	    }

	  if( IS_SET( pexit->to_room->room_flags, ROOM_DARK ) )
	    {
	      visibility--;
	      continue;
	    }

	  ch->in_room = pexit->to_room;
	  if( IS_OUTSIDE(ch) ? distance > visibility : distance > 4 )
	    break;

	  list = ch->in_room->people;
	  for( rch = list; rch != NULL; rch = rch->next_in_room )
	    if( can_see( ch, rch ) )
	      {
		found = TRUE;
		sprintf( buf, "%s%s who is %s %s.\n\r",
			PERS( rch, ch ),
			IS_NPC(rch) ? "" : " (PLAYER)",
			dist_name[distance],
			dir_desc[door] );
		buf[0] = UPPER(buf[0]);
		send_to_char(AT_WHITE, buf, ch );
	      }
	}
    }

  ch->in_room = was_in_room;

  if( !found )
    {
      sprintf( buf, "You see can't see anything %s.\n\r",
	      dir_desc[door] );
      send_to_char(AT_WHITE, buf, ch );
    }

  return;
}



/*
 * Scan command.
 * (by Mikko Kilpikoski 12-Jun-94)
 */
int skill_scan( int sn, int level, CHAR_DATA *ch, void *vo )
{
  int dir;
  bool found;
  if( !check_blind(ch) )
    return SKPELL_MISSED;
    
  if ( IS_NPC( ch ) )
    return SKPELL_MISSED;

  if ( ( ch->level < skill_table[sn].skill_level[ch->class] )
	&& (ch->level < skill_table[sn].skill_level[ch->multied]))
  {
	send_to_char(C_DEFAULT, "Huh?\n\r", ch );
	return SKPELL_MISSED;
  }

  if ( number_percent ( ) > ( ch->pcdata->learned[sn] / 10 )  )
  {
	send_to_char(C_DEFAULT, "Huh?\n\r", ch );
	return SKPELL_MISSED;
  }
    

  if( target_name[0] == '\0' )
    {
      if(ch->class != 10 && ch->multied != 10)
      {
	act(AT_WHITE, "$n scans intensly all around.", ch, NULL, NULL, TO_ROOM );
      }
      else
      {
	act(AT_WHITE, "$n sniffs the air, searching.", ch, NULL, NULL, TO_ROOM );
      }

      found = FALSE;
      for( dir = 0; dir <= 5; dir++ )
	if( ( ch->in_room->exit[dir] != NULL ) 
            && ( !IS_SET(ch->in_room->exit[dir]->exit_info, EX_HIDDEN)  /*Hidden exits by Manaux. */
               || IS_SET(race_table[ch->race].race_abilities, RACE_DETECT_HIDDEN_EXIT) ) )
	  {
	    scan( ch, dir );
	    found = TRUE;
	  }

      if( !found )
	send_to_char(AT_WHITE, "There are no exits here!\n\r", ch );
    }
  else
    {
	   if( !str_prefix( target_name, "north" ) ) dir = DIR_NORTH;
      else if( !str_prefix( target_name, "east"  ) ) dir = DIR_EAST;
      else if( !str_prefix( target_name, "south" ) ) dir = DIR_SOUTH;
      else if( !str_prefix( target_name, "west"  ) ) dir = DIR_WEST;
      else if( !str_prefix( target_name, "up"    ) ) dir = DIR_UP;
      else if( !str_prefix( target_name, "down"  ) ) dir = DIR_DOWN;
      else
	{
	  send_to_char(AT_WHITE, "That's not a direction!\n\r", ch );
	  return SKPELL_NO_DAMAGE;
	}

      if (ch->class != 10 && ch->multied != 10 )
      {
	act(AT_WHITE, "$n scans intensly %t.", ch, NULL, dir_desc[dir], TO_ROOM );
      }
      else
      {
	act(AT_WHITE, "$n sniffs the air to the %t.", ch, NULL, dir_desc[dir], TO_ROOM );
      }

      if( ch->in_room->exit[dir] == NULL )
	{
	  send_to_char(AT_WHITE, "There is no exit in that direction!\n\r", ch );
	  return SKPELL_NO_DAMAGE;
	}

      scan( ch, dir );
    }

  return SKPELL_NO_DAMAGE;
}
