%%% TODO: need to figure out this 'inet6' setting in the tcp options:
%%% It (and *not* 'inet') seems to be necessary when I run on Mac.
%%% Things work with neither setting on Linux and Windows -- not sure
%%% if things work there with the setting though.

%%% When the manager receives a command to install a munger, it can
%%% only affect an existing connection, because the way it works is to
%%% send the info to the registered path_mgr processes.  It doesn't
%%% save up a record of active mungers to bequeath to any connections
%%% that become established after that.
%%%
%%% I think that's a shortcoming that should be fixed.
%%%
%%% We might also need a way to remove a munger, although not
%%% currently.

%%% Hmm, is there any reason why the various path pairs need be
%%% related to each other?  Would it make sense to start [{7001,6001},
%%% {7000,6000}] first, and then later add {7002,6002} to the chaos?
%%% I think that might be fine, but only if the site at 6002 truly
%%% isn't running yet, because if it is, it could send a message that
%%% would need munging.  Similarly, it's important that no running
%%% site have that in its configuration yet, even if the new site
%%% isn't running yet.
%%%
%%% It's not needed yet, but I could imagine a need to target a
%%% command to an as yet unestablished connection/path.  Perhaps it
%%% could be directed to 'new'(?), or {6001,6000,new} (?).  Somewhere
%%% we'd need to hold onto that list, and give it (or some part of it)
%%% to new connections as they're getting established.  The point is,
%%% if you wait until after the connection is established and the
%%% sites have talked to each other, it might be too late to do what
%%% you need to do.


%%% Need to be able to:
%%%   2. remove previously installed munges (shall we give each one a
%%%      serial number or something?)
%%%   3. sometimes it's a little more complicated, and we need a way
%%%      to send a message or signal to an existing munge and/or even
%%%      the recv/send-er (in the case of socket congestion blockage,
%%%      when we want to free it up again).

%%% An interesting feature, that could be useful, maybe even
%%% essential: we could keep a record of all connections, even after
%%% they've been closed, for the purpose of analysis by the test.
%%% Obviously they would be keyed by site-pair (as expressed by port
%%% numbers, similarly to how we addressing path-specific munger
%%% commands).  But they could also include establishment timestamp,
%%% so that a test could distinguish between possibly multiple paths
%%% between the same endpoints.
%%%
%%% With that, a command could request that a path count various
%%% statistics about the messages that pass through (e.g., did we see
%%% any heartbeats on this connection?  We shouldn't, if we're in
%%% mixed-version mode.  Although I suppose an old site would simply
%%% crash if we made that mistake.)  A later command could then come
%%% back and request these statistics.

%% Also, it may be that the forward port isn't listening yet, in which
%% case we could get econnrefused.  Again, it's not terribly
%% disastrous to just let the error report close the connection.  But
%% we should handle it.  Besides, better than closing the initiating
%% connection might be to not even start listening until we see that
%% our forward port is listening.  That's more difficult, and could
%% conceivably bother the target site, but it's more realistic in
%% terms of what the connecting site should see.  Hmm, hard to know
%% which way is better, and whether it's even worth worrying about.
%%
%% Well, here's another thing to think about: it probably doesn't work
%% merely to avoid listening in the beginning, because the site could
%% die in the middle.  We certainly want ultimately to be able to test
%% those kinds of situations.  Hmm, at least for now, perhaps it's all
%% right just to close the incoming connection when we can't make the
%% outgoing connection, because repmgr doesn't really make much
%% difference between an EOF and an error.  But someday it might make
%% a difference.
%%
%% Test functions:
%% - stop reading (to block progress)
%% - discard everything (heartbeats)
%% - discard acks
%% - delay acks
%%
%% Each pair of sites (each link we could care about controlling) has
%% two possible ways it might get set up (in the most general case,
%% though often it's easy enough to control it more strictly).
%%
%% A munge function can be specified as applying only to a specified
%% path, or to all paths.  For example,
%%
%%     {{6000,6001},page_clog}
%%
%% says to apply the page_clog function to the path going from the
%% site listening on 6000 to the site listening on 6001 (both expressed
%% as real port numbers, not the spoofed port numbers).
%%
%%
%% There's another, rather different way of looking at how this gets
%% configured: instead of each site having one fiddler "sheilding" its
%% incoming connections, you could have fiddlers at just one site,
%% completely "wrapping" it, so that it takes not only all incoming
%% connections, but outgoing connections as well.  Consequences: (1)
%% it really focuses on that one site, making it the site under test
%% -- all the others are just going along for the ride.  You could
%% even imagine making them fake.  (2) Other sites talk directly to
%% each other, so we have no control over traffic between them.
%%
%% Site A:
%%     local:  6000
%% Site B:
%%     local:  6001
%%     remote: 7000
%% Site C:
%%     local:  6002
%%     remote: 6000
%% [{7001,6001},{7000,6000},{7002,6002}
%%
%% However, this might be confusing, and is probably counter to some
%% of the higher-level assumptions I've made in setting up
%% transformations.


-module(fiddler).

-export([start/1, start/2, main/1, do_accept/2, slave/2]).
-import(lists,[member/2]).
-import(gen_tcp,[listen/2,accept/1,connect/3,send/2]).

-define(MANAGER_PORT, 8000).

-include("rep_literals.hrl").

%% Config is a list of {spoofed,real} port numbers.  (For now
%% everything's on localhost.)
%%
%% TODO: shouldn't we use records for those tuples?

start(Config) ->
    start(?MANAGER_PORT, Config).

%% For each pair specified in the config, spawn off a listener to
%% spoof the given pair.
%%
start(MgrPort, Pairs) ->
    optstore:start_link(),
    registry:start(),
    manager:start(MgrPort, Pairs),
    lists:foreach(fun (Pair) -> spawn(fiddler, main, [Pair]) end, Pairs).

main(Ports) ->
    {Spoofed, Real} = Ports,
    {ok, LSock} = listen(Spoofed, 
                         [binary, inet, inet6, {packet,raw}, 
                          {active, false}, {reuseaddr, true}]),
    do_accept(LSock, Real).

%% It may seem more straightforward conceptually to spawn a new
%% process to take care of each incoming connection, handing off the
%% socket to the new process, and just have the initial listen process
%% loop back onto the next accept() call.  But instead we "chain" to a
%% new process to continue listening for new connections, and allow
%% this process to proceed to handling socket we've just gotten.  The
%% reason this is convenient is that this process is the "controlling
%% process" of the socket, which means that the socket will
%% automatically get closed when the process terminates.
%% 
do_accept(LSock, RealPort) ->
    {ok, Sock} = accept(LSock),
    Pid = spawn(fiddler, do_accept, [LSock, RealPort]),
    gen_tcp:controlling_process(LSock, Pid),
    slave(Sock, RealPort).

slave(Sock, RealPort) ->
    {ok, TargetSock} = connect("localhost", RealPort,
                               [binary, inet, inet6, {packet, raw}, {active, false}]),

    %% Conduct the initial handshake exchange synchronously, so that
    %% we can easily get the originator's (peer's) port without
    %% programming a FSM:
    %%
    %% (1) pass through the originator's version proposal
    send_msg(TargetSock, reader:read(Sock)),
    %% (2) pass through our local site's version confirmation
    send_msg(Sock, reader:read(TargetSock)),
    %% (3) get the final Parameters handshake, and look there for the
    %% peer port before forwarding it
    ThirdMsg = reader:read(Sock),
    Opts = case ThirdMsg of
               {?HANDSHAKE,_,_,Control,_} ->
                   <<PeerPort:16, _/binary>> = Control,
                   Path = {RealPort, PeerPort},
                   registry:register(Path),
                   initial_opts(Path);
               _ ->
                   []                           % could be JOIN_REQUEST
           end,
    R1 = reader:start(Sock),
    R2 = reader:start(TargetSock),
    send_msg(TargetSock, ThirdMsg),
    reader:prime(R1),
    reader:prime(R2),
    loop(Opts, [{R1,Sock},{R2,TargetSock}]).

%% This is a bit confusing, because there are really 3 port numbers
%% we're interested in.  There's (1) the real port that our local site
%% (the one we're fronting) is actually listening on; (2) the
%% fake/spoof port that *we're* listening on, on our local site's
%% behalf, to which any remote sites are going to be redirected to
%% connect to; and (3) the (nominal/real/configured) port of any
%% remote site that actually does connect to us, which we discover by
%% spying on the handshake message.  The only reason we even care
%% about (3) at all is because it's convenient if we make ourselves
%% known by a "path" 2-tuple that includes it -- i.e., convenient for
%% tests that are using this thing, and want to refer to paths using
%% each site's "real" port numbers.
%%
%% (However, once we get down this far, having already (1) checked for
%% "initial options" and (2) registered ourselves for
%% later/dynamically added options, I suspect we don't even care about
%% *any* of these port values.  All we care about is readers and
%% sockets.

loop(Opts0, Readers) ->
    receive
        {msg,Rdr,Msg} ->
            Opts = check_toss(check_wedge(Opts0, Msg)),
            Tossing = member(toss_all, Opts),
            Wedged = member(wedged, Opts),
            if
                Wedged ->
                    ok;
                Tossing ->
                    reader:prime(Rdr);
                true ->
                    send_msg(other_socket(Readers,Rdr), Msg),
                    reader:prime(Rdr)
            end,
            loop(Opts, Readers);
        {closed,Rdr} ->
            Opts = check_toss(Opts0),
            Tossing = member(toss_all, Opts),
            if Tossing ->
                    {value, {_,S}, Readers2} = lists:keytake(Rdr,1,Readers),
                    gen_tcp:close(S),
                    loop(Opts, Readers2);
                true ->
                    shutdown(Readers)
            end;
        shutdown ->
            shutdown(Readers)
    end.

shutdown(Readers) ->
    lists:foreach(fun ({_R,S}) -> 
                          gen_tcp:close(S)
                  end, Readers).
    

%% Hmm, maybe what we should really do is filter out the matching list
%% element, and then take from what's left.  Might be more code, but
%% more elegant?
%% 
other_socket([{Rdr,_},{_,S}], Rdr) ->
    S;

other_socket([{_,S},{Rdr,_}], Rdr) ->
    S;

other_socket(_,_) ->
    nil.

initial_opts(Path) ->
    optstore:lookup(Path).

%% If we don't already have 'toss' on our list of processing options,
%% then check to see if we're now being instructed to add it, if so
%% then do so.
%% 
check_toss(Opts) ->
    Tossing = member(toss, Opts),
    if
        Tossing ->
            Opts;
        true ->
            receive
                toss_all ->
                    [ toss_all | Opts ]
            after 0 ->
                    Opts
            end
    end.

check_wedge(Opts, Msg) ->
    Wedgeable = member(page_clog, Opts),
    Wedging = member(wedged, Opts),
    if
        Wedgeable andalso not Wedging ->
            case Msg of
                {?REP_MESSAGE,_,_,Control,_Rec} ->
                    <<_:4/unit:32, Type:32/big, _/binary>> = Control,
                    if
                        Type == ?PAGE ->
                            [ wedged | Opts ];
                        true ->
                            Opts
                    end;
                _ ->
                    Opts
            end;
        true ->
            Opts
    end.                    

send_msg(Sock, {MsgType, ControlLength, RecLength, Control, Rec}) ->
    Header = <<MsgType:8, ControlLength:32/big, RecLength:32/big>>,
    send(Sock, Header),
    send_piece(Sock, Control),
    send_piece(Sock, Rec);
send_msg(Sock, {MsgType, Length, Other, Data}) ->
    Header = <<MsgType:8, Length:32/big, Other:32/big>>,
    send(Sock, Header),
    send_piece(Sock, Data);
send_msg(Sock, {MsgType, Length, Other}) ->
    Header = <<MsgType:8, Length:32/big, Other:32/big>>,
    send(Sock, Header);
send_msg(_Sock, nil) ->
    ok.

send_piece(_Sock, nil) ->
    ok;
send_piece(Sock, Piece) ->
    send(Sock, Piece).

