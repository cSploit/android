--
-- Test Statistics Collector
--
-- Must be run after tenk2 has been created (by create_table),
-- populated (by create_misc) and indexed (by create_index).
--

-- conditio sine qua non
SHOW track_counts;  -- must be on

-- wait to let any prior tests finish dumping out stats;
-- else our messages might get lost due to contention
SELECT pg_sleep(2.0);

-- save counters
CREATE TEMP TABLE prevstats AS
SELECT t.seq_scan, t.seq_tup_read, t.idx_scan, t.idx_tup_fetch,
       (b.heap_blks_read + b.heap_blks_hit) AS heap_blks,
       (b.idx_blks_read + b.idx_blks_hit) AS idx_blks
  FROM pg_catalog.pg_stat_user_tables AS t,
       pg_catalog.pg_statio_user_tables AS b
 WHERE t.relname='tenk2' AND b.relname='tenk2';

-- function to wait for counters to advance
create function wait_for_stats() returns void as $$
declare
  start_time timestamptz := clock_timestamp();
  updated bool;
begin
  -- we don't want to wait forever; loop will exit after 30 seconds
  for i in 1 .. 300 loop

    -- check to see if indexscan has been sensed
    SELECT (st.idx_scan >= pr.idx_scan + 1) INTO updated
      FROM pg_stat_user_tables AS st, pg_class AS cl, prevstats AS pr
     WHERE st.relname='tenk2' AND cl.relname='tenk2';

    exit when updated;

    -- wait a little
    perform pg_sleep(0.1);

    -- reset stats snapshot so we can test again
    perform pg_stat_clear_snapshot();

  end loop;

  -- report time waited in postmaster log (where it won't change test output)
  raise log 'wait_for_stats delayed % seconds',
    extract(epoch from clock_timestamp() - start_time);
end
$$ language plpgsql;

-- do a seqscan
SELECT count(*) FROM tenk2;
-- do an indexscan
SELECT count(*) FROM tenk2 WHERE unique1 = 1;

-- force the rate-limiting logic in pgstat_report_tabstat() to time out
-- and send a message
SELECT pg_sleep(1.0);

-- wait for stats collector to update
SELECT wait_for_stats();

-- check effects
SELECT st.seq_scan >= pr.seq_scan + 1,
       st.seq_tup_read >= pr.seq_tup_read + cl.reltuples,
       st.idx_scan >= pr.idx_scan + 1,
       st.idx_tup_fetch >= pr.idx_tup_fetch + 1
  FROM pg_stat_user_tables AS st, pg_class AS cl, prevstats AS pr
 WHERE st.relname='tenk2' AND cl.relname='tenk2';
SELECT st.heap_blks_read + st.heap_blks_hit >= pr.heap_blks + cl.relpages,
       st.idx_blks_read + st.idx_blks_hit >= pr.idx_blks + 1
  FROM pg_statio_user_tables AS st, pg_class AS cl, prevstats AS pr
 WHERE st.relname='tenk2' AND cl.relname='tenk2';

-- End of Stats Test
