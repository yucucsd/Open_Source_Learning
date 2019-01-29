
int is_leap_year(time_t year)
{
	if (year % 4) 
		return 0;
	else if (year % 100)
		return 1;
	else if (year % 400)
		return 0;
	else
		return 1;
}

void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst)
{
	const time_t secs_min = 60;
	const time_t secs_hour = 3600;
	const time_t secs_day = 3600 * 24;
	
	t -= tz;		            /* adjust for timezone. */
	t += 3600 * dst;                    /* adjust for daylight time. */
	time_t days = t / secs_day;         /* Days passed since epoch. */
	time_t seconds = t % secs_day;      /* Remaining seconds. */

	tmp->tm_isdst = dst;
	tmp->tm_hour = seconds / secs_hour;
	tmp->tm_min = (seconds % secs_hour) / secs_min;
	tmp->tm_sec = (seconds % secs_hour) % secs_min;

	/* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
	where sunday = 0, so to calculate the day of week, we have to add 4 and take
	the modulo by 6. */
	tmp->tm_wday = (days + 4) % 7;
	tmp->tm_year = 1970;	
	while(1)
	{
		/* Leap years have one day more. */
		time_t days_this_year = 365 + is_leap_year(temp->tm_year);
		if (days_this_year > days)
			break;
		days -= days_this_year;
		tmp->tm_year++;
	}
	tmp->tm_yday = days; /* Numebr of day of the current year. */

	/* We need to calculate in which month and day of the month we are. To do 
	so, we need to skip days according to how many days there are in each month,
	and adjust for the leap year. */
	int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	mdays[1] += is_leap_year(temp->tm_year);

	tmp->tm_mon = 0;
	while (days >= mdays[tmp->tm_mon])
	{
		days -= mdays[tmp->tm_mon];
		tmp->tm_mon++;
	}
	tmp->tm_mday = days + 1;
	tmp->tm_year -= 1900;
}
