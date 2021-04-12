/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2017 - Jean-André Santoni
 *  Copyright (C) 2015-2018 - Andre Leiradella
 *  Copyright (C) 2018-2020 - natinusala
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../gfx_display.h"
#include "../gfx_widgets.h"

#include "../cheevos/badges.h"

#ifdef HAVE_THREADS
#define SLOCK_LOCK(x) slock_lock(x)
#define SLOCK_UNLOCK(x) slock_unlock(x)
#else
#define SLOCK_LOCK(x)
#define SLOCK_UNLOCK(x)
#endif

#define CHEEVO_NOTIFICATION_DURATION      4000

#define CHEEVO_QUEUE_SIZE 8

struct gfx_widget_achievement_popup_state
{
   gfx_timer_t timer;
   float unfold;
   float y;

   unsigned width;
   unsigned height;

   cheevo_popup queue[CHEEVO_QUEUE_SIZE];
   int queue_read_index;
   int queue_write_index;

#ifdef HAVE_THREADS
   slock_t* queue_lock;
#endif
};

typedef struct gfx_widget_achievement_popup_state gfx_widget_achievement_popup_state_t;

static gfx_widget_achievement_popup_state_t p_w_achievement_popup_st;

static gfx_widget_achievement_popup_state_t* gfx_widget_achievement_popup_get_ptr(void)
{
   return &p_w_achievement_popup_st;
}

/* Forward declarations */
static void gfx_widget_achievement_popup_start(
   gfx_widget_achievement_popup_state_t* state);
static void gfx_widget_achievement_popup_free_current(
   gfx_widget_achievement_popup_state_t* state);

static bool gfx_widget_achievement_popup_init(bool video_is_threaded, bool fullscreen)
{
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();
   memset(state, 0, sizeof(*state));

   state->queue_read_index = -1;

   return true;
}

static void gfx_widget_achievement_popup_free(void)
{
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();

   if (state->queue_read_index >= 0)
   {
      SLOCK_LOCK(state->queue_lock);

      while (state->queue[state->queue_read_index].title)
         gfx_widget_achievement_popup_free_current(state);

      SLOCK_UNLOCK(state->queue_lock);
   }

#ifdef HAVE_THREADS
   slock_free(state->queue_lock);
   state->queue_lock = NULL;
#endif
}

static void gfx_widget_achievement_popup_frame(void* data, void* userdata)
{
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();
   gfx_display_t            *p_disp  = disp_get_ptr();
   gfx_display_ctx_driver_t *dispctx = p_disp->dispctx;

   /* if there's nothing in the queue, just bail */
   if (state->queue_read_index < 0 || !state->queue[state->queue_read_index].title)
      return;

   SLOCK_LOCK(state->queue_lock);

   if (state->queue[state->queue_read_index].title)
   {
      const video_frame_info_t* video_info = (const video_frame_info_t*)data;
      const unsigned video_width = video_info->width;
      const unsigned video_height = video_info->height;

      dispgfx_widget_t* p_dispwidget = (dispgfx_widget_t*)userdata;
      const unsigned unfold_offet = 0;

      int scissor_me_timbers = 0;

      gfx_display_set_alpha(gfx_widgets_get_backdrop_orig(), DEFAULT_BACKDROP);
      gfx_display_set_alpha(gfx_widgets_get_pure_white(), 1.0f);
	  
	    if (dispctx && dispctx->blend_begin)
		   dispctx->blend_begin(video_info->userdata);
	  
	    if (p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_BACKGROUND])
        {
			gfx_widgets_draw_icon(
				video_info->userdata,
				video_width,
				video_height,
				state->width - state->height,
				state->height,
				p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_BACKGROUND],
				video_width - ( video_width * 0.075 ) - state->width,
				state->y + ( video_height * 0.075 ),
				video_width, video_height, 0, 1, gfx_widgets_get_pure_white());		
		}	
		
		if (p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_LEFTEDGE])
        {
			gfx_widgets_draw_icon(
				video_info->userdata,
				video_width,
				video_height,
				state->height,
				state->height,
				p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_LEFTEDGE],
				video_width - video_width * 0.075 - state->height - state->width,
				state->y + ( video_height * 0.075 ),
				video_width, video_height, 0, 1, gfx_widgets_get_pure_white());				
		}	
		
		if (p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_RIGHTEDGE])
        {
			gfx_widgets_draw_icon(
				video_info->userdata,
				video_width,
				video_height,
				state->height,
				state->height,
				p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_RIGHTEDGE],
				video_width - ( video_width * 0.075 ) - state->height,
				state->y + ( video_height * 0.075 ),
				video_width, video_height, 0, 1, gfx_widgets_get_pure_white());
		}	
		
		if (dispctx && dispctx->blend_end)
               dispctx->blend_end(video_info->userdata);	

      /* Default icon */
      if (!state->queue[state->queue_read_index].badge)
      {
         /* Icon */
         if (p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_ACHIEVEMENT])
         {
            gfx_display_blend_begin(userdata);
            gfx_widgets_draw_icon(
               video_info->userdata,
               video_width,
               video_height,
               0.75 * state->height,
               0.75 * state->height,
               p_dispwidget->gfx_widgets_icons_textures[MENU_WIDGETS_ICON_ACHIEVEMENT],
               (video_width - video_width * 0.075 - state->height - state->width) + ( state->height / 8 ),
               state->y + ( video_height * 0.075 ) + ( state->height / 8 ),
               video_width, video_height, 0, 1, gfx_widgets_get_pure_white());
            gfx_display_blend_end(userdata);
         }
      }
      /* Badge */
      else
      {
         gfx_widgets_draw_icon(
            video_info->userdata,
            video_width,
            video_height,
            0.75 * state->height,
            0.75 * state->height,
            state->queue[state->queue_read_index].badge,
            (video_width - video_width * 0.075 - state->height - state->width) + ( state->height / 8 ),
            state->y + ( video_height * 0.075 ) + ( state->height / 8 ),
            video_width,
            video_height,
            0,
            1,
            gfx_widgets_get_pure_white());
      }

      /* Title */
      gfx_widgets_draw_text(&p_dispwidget->gfx_widget_fonts.regular,
         msg_hash_to_str(MSG_ACHIEVEMENT_UNLOCKED),
         video_width - (video_width * 0.075) - state->width,
         (state->y + p_dispwidget->gfx_widget_fonts.regular.line_height
         + p_dispwidget->gfx_widget_fonts.regular.line_ascender) + (video_height * 0.075),
         video_width, video_height,
         TEXT_COLOR_FAINT,
         TEXT_ALIGN_LEFT,
         true);

      /* Cheevo name */

      /* TODO: is a ticker necessary ? */
      gfx_widgets_draw_text(&p_dispwidget->gfx_widget_fonts.regular,
         state->queue[state->queue_read_index].title,
          video_width - (video_width * 0.075) - state->width,
         (state->y + state->height
         - p_dispwidget->gfx_widget_fonts.regular.line_height
         - p_dispwidget->gfx_widget_fonts.regular.line_descender) + (video_height * 0.075),
         video_width, video_height,
         TEXT_COLOR_INFO,
         TEXT_ALIGN_LEFT,
         true);
   }

   SLOCK_UNLOCK(state->queue_lock);
}

static void gfx_widget_achievement_popup_free_current(gfx_widget_achievement_popup_state_t* state)
{
   if (state->queue[state->queue_read_index].title)
   {
      free(state->queue[state->queue_read_index].title);
      state->queue[state->queue_read_index].title = NULL;
   }

   if (state->queue[state->queue_read_index].badge)
   {
      video_driver_texture_unload(&state->queue[state->queue_read_index].badge);
      state->queue[state->queue_read_index].badge = 0;
   }

   state->queue_read_index = (state->queue_read_index + 1) % ARRAY_SIZE(state->queue);
}

static void gfx_widget_achievement_popup_next(void* userdata)
{
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();

   SLOCK_LOCK(state->queue_lock);

   if (state->queue_read_index >= 0)
   {
      gfx_widget_achievement_popup_free_current(state);

      /* start the next popup (if present) */
      if (state->queue[state->queue_read_index].title)
         gfx_widget_achievement_popup_start(state);
   }

   SLOCK_UNLOCK(state->queue_lock);
}

static void gfx_widget_achievement_popup_dismiss(void *userdata)
{
   gfx_animation_ctx_entry_t entry;
   const dispgfx_widget_t *p_dispwidget = (const dispgfx_widget_t*)dispwidget_get_ptr();
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();

   /* Slide up animation */
   entry.cb             = gfx_widget_achievement_popup_next;
   entry.duration       = MSG_QUEUE_ANIMATION_DURATION;
   entry.easing_enum    = EASING_OUT_QUAD;
   entry.subject        = &state->y;
   entry.tag            = p_dispwidget->gfx_widgets_generic_tag;
   entry.target_value   = (float)(-(int)(state->height));
   entry.userdata       = NULL;

   gfx_animation_push(&entry);
}

static void gfx_widget_achievement_popup_fold(void *userdata)
{
   gfx_animation_ctx_entry_t entry;
   const dispgfx_widget_t *p_dispwidget = (const dispgfx_widget_t*)dispwidget_get_ptr();
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();

   /* Fold */
   entry.cb             = gfx_widget_achievement_popup_dismiss;
   entry.duration       = MSG_QUEUE_ANIMATION_DURATION;
   entry.easing_enum    = EASING_OUT_QUAD;
   entry.subject        = &state->unfold;
   entry.tag            = p_dispwidget->gfx_widgets_generic_tag;
   entry.target_value   = 0.0f;
   entry.userdata       = NULL;

   gfx_animation_push(&entry);
}

static void gfx_widget_achievement_popup_unfold(void *userdata)
{
   gfx_timer_ctx_entry_t timer;
   gfx_animation_ctx_entry_t entry;
   const dispgfx_widget_t *p_dispwidget = (const dispgfx_widget_t*)dispwidget_get_ptr();
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();

   /* Unfold */
   entry.cb             = NULL;
   entry.duration       = MSG_QUEUE_ANIMATION_DURATION;
   entry.easing_enum    = EASING_OUT_QUAD;
   entry.subject        = &state->unfold;
   entry.tag            = p_dispwidget->gfx_widgets_generic_tag;
   entry.target_value   = 1.0f;
   entry.userdata       = NULL;

   gfx_animation_push(&entry);

   /* Wait before dismissing */
   timer.cb       = gfx_widget_achievement_popup_fold;
   timer.duration = MSG_QUEUE_ANIMATION_DURATION + CHEEVO_NOTIFICATION_DURATION;
   timer.userdata = NULL;

   gfx_timer_start(&state->timer, &timer);
}

static void gfx_widget_achievement_popup_start(
   gfx_widget_achievement_popup_state_t* state)
{
   const dispgfx_widget_t* p_dispwidget = (const dispgfx_widget_t*)dispwidget_get_ptr();
   gfx_animation_ctx_entry_t entry;

   state->height = p_dispwidget->gfx_widget_fonts.regular.line_height * 4;
   state->width  = MAX(
         font_driver_get_message_width(
            p_dispwidget->gfx_widget_fonts.regular.font,
            msg_hash_to_str(MSG_ACHIEVEMENT_UNLOCKED), 0, 1),
         font_driver_get_message_width(
            p_dispwidget->gfx_widget_fonts.regular.font,
            state->queue[state->queue_read_index].title, 0, 1));
   state->width += p_dispwidget->simple_widget_padding * 2;
   state->y      = (float)(-(int)state->height);
   state->unfold = 0.0f;

   /* Slide down animation */
   entry.cb             = gfx_widget_achievement_popup_unfold;
   entry.duration       = MSG_QUEUE_ANIMATION_DURATION;
   entry.easing_enum    = EASING_OUT_QUAD;
   entry.subject        = &state->y;
   entry.tag            = p_dispwidget->gfx_widgets_generic_tag;
   entry.target_value   = 0.0f;
   entry.userdata       = NULL;

   gfx_animation_push(&entry);
}

void gfx_widgets_push_achievement(const char *title, const char *badge)
{
   gfx_widget_achievement_popup_state_t* state = gfx_widget_achievement_popup_get_ptr();
   int start_notification = 1;

   /* important - this must be done outside the lock because it has the potential to need to
    * lock the video thread, which may be waiting for the popup queue lock to render popups */
   uintptr_t badge_id = cheevos_get_badge_texture(badge, 0);

   if (state->queue_read_index < 0)
   {
      /* queue uninitialized */
      memset(&state->queue, 0, sizeof(state->queue));
      state->queue_read_index = 0;

#ifdef HAVE_THREADS
      state->queue_lock = slock_new();
#endif
   }

   SLOCK_LOCK(state->queue_lock);

   if (state->queue_write_index == state->queue_read_index)
   {
      if (state->queue[state->queue_write_index].title)
      {
         /* queue full */
         SLOCK_UNLOCK(state->queue_lock);
         return;
      }

      /* queue empty */
   }
   else
      start_notification = 0; /* notification already being displayed */

   state->queue[state->queue_write_index].badge = badge_id;
   state->queue[state->queue_write_index].title = strdup(title);

   state->queue_write_index = (state->queue_write_index + 1) % ARRAY_SIZE(state->queue);

   if (start_notification)
      gfx_widget_achievement_popup_start(state);

   SLOCK_UNLOCK(state->queue_lock);
}

const gfx_widget_t gfx_widget_achievement_popup = {
   &gfx_widget_achievement_popup_init,
   &gfx_widget_achievement_popup_free,
   NULL, /* context_reset*/
   NULL, /* context_destroy */
   NULL, /* layout */
   NULL, /* iterate */
   &gfx_widget_achievement_popup_frame
};
