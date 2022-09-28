#include "Twitch.hpp"
#include <iostream>
#include <regex>
#include <fstream>
#include <mutex>
#include <map>
#include <cctype>

#include <cassert>

#include "libircclient.h"
#include "libirc_rfcnumeric.h"
#include "..\mods\Twitch.hpp"

#define TWITCH_DEBUG
// Function Types

// those are done to remove dependency on libircclient.dll
// building static .lib file is too much of a pain in the ass
// and using provided binaries makes us depend on libircclient.dll
// being present in the game directory even if you dont want twitch
// shit in your game, preventing the game from starting. 
// We'll attempt to link stuff dynamically using function pointers
// and function stubs to work around this using GetProcAddress(),
// with good old preprocessor abuse. should be working unless
// i forgot how to do this

// irc_create_session
#define IRC_CREATE_SESSION(name) \
  irc_session_t * name(irc_callbacks_t	* callbacks)
typedef IRC_CREATE_SESSION(irc_create_session_);

IRC_CREATE_SESSION(IRCCreateSessionStub) { return 0; }
static irc_create_session_ *IRCCreateSession_ = IRCCreateSessionStub;

#define irc_create_session IRCCreateSession_

// irc_connect
#define IRC_CONNECT(name) \
  int name (irc_session_t * session, \
const char * server, \
unsigned short port, \
const char * server_password, \
const char * nick, \
const char * username, \
const char * realname)
typedef IRC_CONNECT(irc_connect_);

IRC_CONNECT(IRCConnectStub) { return 0; }
static irc_connect_ *IRCConnect_ = IRCConnectStub;

#define irc_connect IRCConnect_

//irc_disconnect
#define IRC_DISCONNECT(name) \
  void name (irc_session_t * session)
typedef IRC_DISCONNECT(irc_disconnect_);

IRC_DISCONNECT(IRCDisconnectStub) { return; }
static irc_disconnect_ *IRCDisconnect_ = IRCDisconnectStub;

#define irc_disconnect IRCDisconnect_

//irc_run
#define IRC_RUN(name) \
  int name (irc_session_t * session)
typedef IRC_RUN(irc_run_);

IRC_RUN(IRCRunStub) { return 0; }
static irc_run_ *IRCRun_ = IRCRunStub;

#define irc_run IRCRun_

//irc_cmd_join
#define IRC_CMD_JOIN(name) \
  int name (irc_session_t * session, const char * channel, const char * key)
typedef IRC_CMD_JOIN(irc_cmd_join_);

IRC_CMD_JOIN(IRCCmdJoinStub) { return 0; }
static irc_cmd_join_ *IRCCmdJoin_ = IRCCmdJoinStub;

#define irc_cmd_join IRCCmdJoin_

//irc_cmd_msg
#define IRC_CMD_MSG(name) \
  int name  (irc_session_t * session, const char * nch, const char * text)
typedef IRC_CMD_MSG(irc_cmd_msg_);

IRC_CMD_MSG(IRCCmdMsgStub) { return 0; }
static irc_cmd_msg_ *IRCCmdMsg_ = IRCCmdMsgStub;

#define irc_cmd_msg IRCCmdMsg_

//irc_set_ctx
#define IRC_SET_CTX(name) \
  void name (irc_session_t * session, void * ctx)
typedef IRC_SET_CTX(irc_set_ctx_);

IRC_SET_CTX(IRCSetCtxStub) { return; }
static irc_set_ctx_ *IRCSetCtx_ = IRCSetCtxStub;

#define irc_set_ctx IRCSetCtx_

//irc_get_ctx
#define IRC_GET_CTX(name) \
  void * name (irc_session_t * session)
typedef IRC_GET_CTX(irc_get_ctx_);

IRC_GET_CTX(IRCGetCtxStub) { return 0; }
static irc_get_ctx_ *IRCGetCtx_ = IRCGetCtxStub;

#define irc_get_ctx IRCGetCtx_

//irc_errno
#define IRC_ERRNO(name) \
  int name (irc_session_t * session)
typedef IRC_ERRNO(irc_errno_);

IRC_ERRNO(IRCErrnoStub) { return 0; }
static irc_errno_ *IRCErrno_ = IRCErrnoStub;

#define irc_errno IRCErrno_


//irc_strerror
#define IRC_STRERROR(name) \
  const char * name (int ircerrno)
typedef IRC_STRERROR(irc_strerror_);

IRC_STRERROR(IRCStrErrorStub) { return "error"; }
static irc_strerror_ *IRCStrError_ = IRCStrErrorStub;

#define irc_strerror IRCStrError_

//irc_option_set
#define IRC_OPTION_SET(name) \
  void name (irc_session_t * session, unsigned int option)
typedef IRC_OPTION_SET(irc_option_set_);

IRC_OPTION_SET(IRCOptionSetStub) { return; }
static irc_option_set_ *IRCOptionSet_ = IRCOptionSetStub;

#define irc_option_set IRCOptionSet_

// dynamically link irc functions
size_t dyn_link_lib_irc_client() {
	HMODULE library = LoadLibraryA("libircclient.dll");
	if (library) {
		irc_create_session = (irc_create_session_ *)GetProcAddress(library, "irc_create_session");
		assert(irc_create_session != NULL);
		irc_connect = (irc_connect_ *)GetProcAddress(library, "irc_connect");
		assert(irc_connect != NULL);
		irc_disconnect = (irc_disconnect_ *)GetProcAddress(library, "irc_disconnect");
		assert(irc_disconnect != NULL);
		irc_run = (irc_run_ *)GetProcAddress(library, "irc_run");
		assert(irc_run != NULL);
		irc_cmd_join = (irc_cmd_join_ *)GetProcAddress(library, "irc_cmd_join");
		assert(irc_cmd_join != NULL);
		irc_cmd_msg = (irc_cmd_msg_ *)GetProcAddress(library, "irc_cmd_msg");
		assert(irc_cmd_msg != NULL);
		irc_set_ctx = (irc_set_ctx_ *)GetProcAddress(library, "irc_set_ctx");
		assert(irc_set_ctx != NULL);
		irc_get_ctx = (irc_get_ctx_ *)GetProcAddress(library, "irc_get_ctx");
		assert(irc_get_ctx != NULL);
		irc_errno = (irc_errno_ *)GetProcAddress(library, "irc_errno");
		assert(irc_errno != NULL);
		irc_strerror = (irc_strerror_ *)GetProcAddress(library, "irc_strerror");
		assert(irc_strerror != NULL);
		irc_option_set = (irc_option_set_ *)GetProcAddress(library, "irc_option_set");
		assert(irc_option_set != NULL);

		return (size_t)library; //success, not sucks ass
	}
	return NULL; // sucks ass
}

void event_connect( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {
	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}

	if ( irc_cmd_join( session, ctx->twitch->channel.c_str(), 0 ) ) {
		ctx->twitch->on_error( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
		return;
	}

	ctx->twitch->status = TWITCH_CONNECTED;
	ctx->twitch->on_connected();
}

void event_channel( irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count ) {

	TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
	if ( !ctx ) {
		return;
	}

	std::string original_message = params[1];
	ctx->twitch->on_message( origin, original_message );
	ctx->twitch->messages.emplace_back( std::pair{ original_message, origin } );

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
		ctx->twitch->on_error( event, fulltext );
	}
}
/*
Twitch::Twitch() {
}
*/

Twitch::~Twitch() {
	if ( session ) {
		TwitchContext *ctx = ( TwitchContext * ) irc_get_ctx( session );
        delete ctx;

		disconnect();
	}
}

std::thread Twitch::connect( const std::string &user, const std::string &password) {
	return std::thread( [this, user, password] {
		std::string lowercase_name = user;
		std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(),
			[](unsigned char c){ return std::tolower(c); });
		this->user = lowercase_name;
		this->channel = "#" + lowercase_name;

		irc_callbacks_t	callbacks;
		memset( &callbacks, 0, sizeof( callbacks ) );
		callbacks.event_connect = event_connect;
		callbacks.event_channel = event_channel;
		callbacks.event_numeric = event_numeric;

		session = irc_create_session( &callbacks );

		if ( !session ) {
			on_error( -1, "Failed to create IRC session" );
			return;
		}

		TwitchContext *ctx = new TwitchContext();
		ctx->twitch = this;
		irc_set_ctx( session, ctx );

		irc_option_set( session, LIBIRC_OPTION_STRIPNICKS );

		status = TWITCH_CONNECTING;
#ifdef TWITCH_DEBUG
        std::string tuser = "dobsmith"; std::string tpassword = "hehe";
        if (irc_connect(session, "127.0.0.1", 6667, tpassword.c_str(), tuser.c_str(), tuser.c_str(), tuser.c_str())) {
#else
		if (irc_connect(session, "irc.chat.twitch.tv", 6667, password.c_str(), user.c_str(), user.c_str(), user.c_str())) {
#endif
			on_error(irc_errno(session), irc_strerror(irc_errno(session)));
			status = TWITCH_DISCONNECTED;
			return;
		}

		if ( irc_run( session ) ) {
			on_error( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
			status = TWITCH_DISCONNECTED;
			return;
		}
	} );
}

void Twitch::disconnect() {
	if ( session ) {
		irc_disconnect( session );

		status = TWITCH_DISCONNECTED;
		on_disconnected();
	}
}

void Twitch::send_chat_message( const std::string &message ) {
	if ( !session ) {
		return;
	}

	if ( irc_cmd_msg( session, channel.c_str(), message.c_str() ) ) {
		on_error( irc_errno( session ), irc_strerror( irc_errno( session ) ) );
	}
}