-module(util).
-export([match/2]).

match({P1,P2}, {P1,P2}) ->
    true;

match({P1,P2}, {P2,P1}) ->
    true;

match(_,_) ->
    false.
