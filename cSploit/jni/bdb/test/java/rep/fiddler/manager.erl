%%% Manager is a process that reads commands from a telnet-like
%%% socket, primarily to add mungers to the Registry under control of
%%% a test (though it could be more generally anything).

-module(manager).
-export([start/2,accept_loop/2,recv_loop/2]).
-import(gen_tcp, [listen/2, accept/1, recv/2, send/2]).
-import(string, [cspan/2, substr/3]).

start(Port, Config) ->
    {ok, LSock} = listen(Port, [{packet,line},
                                {active,false},{reuseaddr,true}]),
    Pid = spawn(?MODULE, accept_loop, [LSock, Config]),
    ok = gen_tcp:controlling_process(LSock, Pid).

accept_loop(LSock, Config) ->
    {ok, Sock} = accept(LSock),
    spawn(?MODULE, recv_loop, [Sock, Config]),
    accept_loop(LSock, Config).

recv_loop(Sock, Config) ->
    case recv(Sock, 0) of
        {ok, Msg} ->
            %%
            %% Take the input, strip off CRLF, slap on a dot so as to
            %% make it nice for Erlang parser.
            %% 
            ValidInput = lists:append(substr(Msg, 1, cspan(Msg, "\r\n")),
                                      "."),
            {ok,Tokens,_} = erl_scan:string(ValidInput),
            {ok, Result} = erl_parse:parse_term(Tokens),
            case Result of
                {init, Path, Opt} ->
                    {ok, Num} = optstore:install(Path, Opt),
                    send(Sock, integer_to_list(Num)),
                    send(Sock, "\r\n");

                {Path, Command} when is_tuple(Path) ->
                    Pids = registry:lookup(Path),
                    lists:foreach(fun(Pid) -> Pid ! Command end, Pids);
                shutdown ->
                    send(Sock, "ok\r\n"),
                    halt();                     % first try is rather crude
                {config,Real} ->
                    send(Sock,
                         integer_to_list(
                           case lists:keysearch(Real, 2, Config) of
                               {value,{Spoof,Real}} -> Spoof;
                               false -> Real
                           end)),
                    send(Sock, "\r\n");
                list ->
                    lists:foreach(fun ({Id,Path,_,_,_}) ->
                                          {From,To} = Path,
                                          send(Sock, integer_to_list(Id)),
                                          send(Sock, " {"),
                                          send(Sock, integer_to_list(From)),
                                          send(Sock, ","),
                                          send(Sock, integer_to_list(To)),
                                          send(Sock, "}\r\n")
                                  end, registry:all());

                Command ->
                    %%
                    %% Each half of a bidirectional path gets
                    %% registered to the same path_mgr process.  So we
                    %% end up with duplicates if we just look at the list
                    %% of Pids.
                    %%
                    All = registry:all(),
                    AllPids = lists:map(fun (X) -> element(5, X) end, All),
                    DistinctPids = lists:usort(AllPids),
                    lists:foreach(fun (P) -> P ! {munger,Command} end, 
                                  DistinctPids)
            end,
            send(Sock, "ok\r\n"),
            recv_loop(Sock, Config);
        {error,closed} ->
            ok
    end.


%%% {{from,to},shutdown} -> look up the sockets, and close them
%%% 
