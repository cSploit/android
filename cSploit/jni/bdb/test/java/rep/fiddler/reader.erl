-module(reader).

-export([read/1,start/1,prime/1,loop/1]).
-import(gen_tcp, [recv/2]).

-include("rep_literals.hrl").

read(Sock) ->
    <<EnvType, Word1:32, Word2:32>> = get_piece(Sock, 9),
    case EnvType of
        T when T == ?ACK;
               T == ?HANDSHAKE;
               T == ?REP_MESSAGE;
               T == ?HEARTBEAT ->
            traditional_msg(Sock, T, Word1, Word2);
        T when T == ?APP_MSG;
               T == ?APP_RESPONSE;
               T == ?OWN_MSG ->
            simple_msg(Sock, T, Word1, Word2);
        T when T == ?RESP_ERROR ->
            {T, Word1, Word2}
    end.

start(Sock) ->
    spawn_link(reader, loop, [Sock]).

prime(Reader) ->
    Reader ! {self(),prime}.

loop(Sock) ->
    receive
        {From,prime} ->
            case read0(Sock) of
                {error,closed} ->
                    From ! {closed,self()};
                Msg ->
                    From ! {msg,self(),Msg},
                    loop(Sock)
            end
    end.

read0(Sock) ->
    catch read(Sock).

traditional_msg(Sock, MsgType, ControlLength, RecLength) ->
    Control = get_piece(Sock, ControlLength),
    Rec = get_piece(Sock, RecLength),
    {MsgType, ControlLength, RecLength, Control, Rec}.

simple_msg(Sock, MsgType, Length, Word2) ->
    Msg = get_piece(Sock, Length),
    {MsgType, Length, Word2, Msg}.

get_piece(_, 0) ->
    nil;

get_piece(S, Length) ->
    case recv(S, Length) of
        {ok, Piece} ->
            Piece;
        {error,closed} ->
            throw({error,closed})
    end.
