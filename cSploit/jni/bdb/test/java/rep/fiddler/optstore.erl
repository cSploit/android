%% Keep track of "initial options" (processing options which can be
%% requested for a particular path, before the path has been
%% established).
%% 
-module(optstore).
-behavior(gen_server).

-export([start_link/0,install/2,remove/1,lookup/1]).
-export([init/1,handle_call/3,handle_cast/2]).
-export([code_change/3,handle_info/2,terminate/2]).

-record(opt, {num, path, token}).

start_link() ->
    gen_server:start_link({local,optstore}, optstore, [], []).

install(Path, Opt) ->
    gen_server:call(optstore, {install,Path,Opt}).

remove(Num) ->
    gen_server:cast(optstore, {remove,Num}).

lookup(Path) ->
    {ok, Ans} = gen_server:call(optstore, {lookup,Path}),
    Ans.

init(_) ->
    {ok, {0,[]}}.

handle_call({install,Path,Opt}, _From, {Count,Opts}) ->
    Num = Count + 1,
    Added = #opt{num=Num, path=Path, token=Opt},
    {reply, {ok, Num}, {Num, [Added | Opts]}};

handle_call({lookup,Path}, _From, State = {_Count,Opts}) ->
    Ans = [ Token || #opt{token=Token,path=P} <- Opts, util:match(Path, P) ],
    {reply, {ok, Ans}, State}.

handle_cast({remove, Num}, {Count, Opts}) ->
    {noreply, {Count, lists:keydelete(Num, #opt.num, Opts)}}.

%%%
%%% The following functions defined in the gen_server behavior are not
%%% used.  But we include them in order to avoid distracting
%%% compilation warnings.
%%% 
code_change(_,_,_) ->
    ok.

handle_info(_,_) ->
    ok.

terminate(_,_) ->
    ok.
