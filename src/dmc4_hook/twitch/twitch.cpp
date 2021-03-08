#include "twitch.hpp"
#include <iostream>
#include <regex>
#include <fstream>
#include <mutex>
#include <map>


std::mutex killfeedMutex;

void event_connect( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {
	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}

	if ( irc_cmd_join( session, ctx->twitch->channel.c_str(), 0 ) ) {
		ctx->twitch->OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
		return;
	}

	ctx->twitch->status = TWITCH_CONNECTED;
	ctx->twitch->OnConnected();
}

void event_channel( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {

	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}

	std::string originalMessage = params[1];
	ctx->twitch->OnMessage( origin, originalMessage );
	ctx->twitch->messages.push_back( { originalMessage, origin } );

}

void event_numeric( irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count ) {
	if ( event > 400 ) {
		std::string fulltext;
		for ( unsigned int i = 0; i < count; i++ ) {
			if ( i > 0 )
				fulltext += " ";

			fulltext += params[i];
		}

		TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
		if ( !ctx ) {
			return;
		}
		ctx->twitch->OnError( event, fulltext );
	}
}

Twitch::Twitch() {
}

Twitch::~Twitch() {
	if ( session ) {
		TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
		if ( ctx ) {
			delete ctx;
		}

		Disconnect();
	}
}

std::thread Twitch::Connect( const std::string &user, const std::string &password ) {
	return std::thread( [this, user, password] {
		this->user = user;
		this->channel = "#" + user;

		irc_callbacks_t	callbacks;
		memset( &callbacks, 0, sizeof( callbacks ) );
		callbacks.event_connect = event_connect;
		callbacks.event_channel = event_channel;
		callbacks.event_numeric = event_numeric;

		session = irc_create_session( &callbacks );

		if ( !session ) {
			OnError( -1, "Failed to create IRC session" );
			return;
		}

		TwitchContext *ctx = new TwitchContext();
		ctx->twitch = this;
		irc_set_ctx( session, ctx );

		irc_option_set( session, LIBIRC_OPTION_STRIPNICKS );

		status = TWITCH_CONNECTING;

		if ( irc_connect( session, "irc.chat.twitch.tv", 6667, password.c_str(), user.c_str(), user.c_str(), user.c_str() ) ) {
			OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
			status = TWITCH_DISCONNECTED;
			return;
		}

		if ( irc_run( session ) ) {
			OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
			status = TWITCH_DISCONNECTED;
			return;
		}
	} );
}

void Twitch::Disconnect() {
	if ( session ) {
		irc_disconnect( session );

		status = TWITCH_DISCONNECTED;
		OnDisconnected();
	}
}

void Twitch::SendChatMessage( const std::string &message ) {
	if ( !session ) {
		return;
	}

	if ( irc_cmd_msg( session, channel.c_str(), message.c_str() ) ) {
		OnError( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
	}
}

