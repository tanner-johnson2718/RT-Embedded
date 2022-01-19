


start_section :

        i           	: integer;
        nb_T2          	: integer;
        nb_T1           : integer;
        jitter_bound : integer;
        max_jitter       : integer;
        min_jitter       : integer;
        tmp             : integer;
        T1_end_time     : array (time_units_range) of integer;
        T2_end_time     : array (time_units_range) of integer;

        min_jitter:=integer'last;
        max_jitter:=integer'first;
        i:=0;
    	nb_T1:=0; nb_T2:=0;

end section;


gather_event_analyzer_section :

        if (events.type = "end_of_task_capacity")
		then
			if (events.task_name = "s0.i.p1.T1")
				then	
                         T1_end_time(nb_T1):=events.time;
				 nb_T1:=nb_T1+1;
			end if;
			if (events.task_name = "s0.i.p1.T2")
				then	
                         T2_end_time(nb_T2):=events.time;
				 nb_T2:=nb_T2+1;
			end if;
	end if;

end section;



display_event_analyzer_section :
              
      while (i<nb_T1) and (i<nb_T2) loop
           tmp:=abs(T1_end_time(i)-T2_end_time(i));
           min_jitter:=min(tmp, min_jitter);
           max_jitter:=max(tmp, max_jitter);  
	     i:=i+1;
      end loop;
      jitter_bound:=abs(max_jitter-min_jitter);        

      put(min_jitter);
      put(max_jitter);
      put(jitter_bound);     

end section;

