#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hcq.h"
#define INPUT_BUFFER_SIZE 256

/*
 * Return a pointer to the struct student with name stu_name
 * or NULL if no student with this name exists in the stu_list
 */
Student *find_student(Student *stu_list, char *student_name) {
    Student *head = stu_list;
    while(head != NULL){
        if( ! strcmp(head -> name, student_name)){
            return head;
        }
        head = head -> next_overall;
    }
    return NULL;
}



/*   Return a pointer to the ta with name ta_name or NULL
 *   if no such TA exists in ta_list. 
 */
Ta *find_ta(Ta *ta_list, char *ta_name) {
    Ta *head = ta_list;
    while(head != NULL){
        if( ! strcmp(head -> name, ta_name)){
            return head;
        }
        head = head -> next;
    }
    return NULL;
}


/*  Return a pointer to the course with this code in the course list
 *  or NULL if there is no course in the list with this code.
 */
Course *find_course(Course *courses, int num_courses, char *course_code) {
    for(int i = 0; i < num_courses; i++){
        if(! strcmp(courses[i].code, course_code)){
            return (courses + i);
        }
    }
    return NULL;
}
    

/* Add a student to the queue with student_name and a question about course_code.
 * if a student with this name already has a question in the queue (for any
   course), return 1 and do not create the student.
 * If course_code does not exist in the list, return 2 and do not create
 * the student struct.
 * For the purposes of this assignment, don't check anything about the 
 * uniqueness of the name. 
 */
int add_student(Student **stu_list_ptr, char *student_name, char *course_code,
    Course *course_array, int num_courses) {

    if( find_student(*stu_list_ptr, student_name) != NULL){
        return 1;
    }
    Course *course;
    if( (course = find_course(course_array, num_courses, course_code)) == NULL){
        return 2;
    }

    Student *new_stu = malloc(sizeof(Student));
    if(new_stu == NULL){
        perror("malloc for Student");
        exit(1);
    }

    //name
    new_stu -> name = malloc(sizeof(char) * (strlen(student_name) + 1));
    if(new_stu -> name == NULL){
        perror("malloc for Student name");
        exit(1);
    }
    strcpy(new_stu -> name, student_name);

    //arrival_time
    new_stu -> arrival_time = malloc(sizeof(time_t));
    if(new_stu -> arrival_time == NULL){
        perror("malloc for Student arrival_time");
        exit(1);
    }
    *(new_stu -> arrival_time) = time(NULL);

    new_stu -> course = course;
    new_stu -> next_overall = NULL;
    new_stu -> next_course = NULL;

    //insert at the end of list
    if(*stu_list_ptr == NULL){
        *stu_list_ptr = new_stu;
    }else{
        Student *stu_head = *stu_list_ptr;
        while(stu_head->next_overall != NULL){
            stu_head = stu_head->next_overall;
        }
        stu_head -> next_overall = new_stu;
    }

    //update course info
    if(course -> head == NULL){
        course -> head = new_stu;
        course -> tail = new_stu;
    }else{
        (course -> tail) -> next_course = new_stu;
        course -> tail = new_stu;
    }
    return 0;
}


/* Student student_name has given up waiting and left the help centre
 * before being called by a Ta. Record the appropriate statistics, remove
 * the student from the queues and clean up any no-longer-needed memory.
 *
 * If there is no student by this name in the stu_list, return 1.
 */
int give_up_waiting(Student **stu_list_ptr, char *student_name) {
    Student *stu;
    if( (stu = find_student(*stu_list_ptr, student_name)) == NULL){
        return 1;
    }

    //update course info
    Course *course = stu -> course;
    course -> bailed ++;
    course -> wait_time += time(NULL) - *(stu -> arrival_time);

    //update course head
    if(stu == course -> head){
        if(stu -> next_course != NULL){
            course -> head = stu -> next_course;
        }else{
            course -> head = NULL;
            course -> tail = NULL;
        }
    //update next_course of previous student & course tail
    }else{
        Student *head = course -> head;
        while(head -> next_course != stu){
            head = head -> next_course;
        }
        head -> next_course = stu -> next_course;
        if(stu == course -> tail){
            course -> tail = head;
        }
    }

    //update stu_list
    if(stu == *stu_list_ptr){
        *stu_list_ptr = stu -> next_overall;
    }else{
        Student *head = *stu_list_ptr;
        while(head -> next_overall != stu){
            head = head -> next_overall;
        }
        head -> next_overall = stu -> next_overall;
    }

    free(stu -> name);
    free(stu -> arrival_time);
    free(stu);

    return 0;
}

/* Create and prepend Ta with ta_name to the head of ta_list. 
 * For the purposes of this assignment, assume that ta_name is unique
 * to the help centre and don't check it.
 */
void add_ta(Ta **ta_list_ptr, char *ta_name) {
    // first create the new Ta struct and populate
    Ta *new_ta = malloc(sizeof(Ta));
    if (new_ta == NULL) {
       perror("malloc for TA");
       exit(1);
    }
    new_ta->name = malloc(strlen(ta_name)+1);
    if (new_ta->name  == NULL) {
       perror("malloc for TA name");
       exit(1);
    }
    strcpy(new_ta->name, ta_name);
    new_ta->current_student = NULL;

    // insert into front of list
    new_ta->next = *ta_list_ptr;
    *ta_list_ptr = new_ta;
}

/* The TA ta is done with their current student. 
 * Calculate the stats (the times etc.) and then 
 * free the memory for the student. 
 * If the TA has no current student, do nothing.
 */
void release_current_student(Ta *ta) {
    Student *stu;
    if( (stu = ta -> current_student) != NULL){
        Course *course = stu -> course;
        course -> help_time += time(NULL) - *(stu -> arrival_time);
        course -> helped += 1;
        free(stu -> name);
        free(stu -> arrival_time);
        free(stu);
    }
}

/* Remove this Ta from the ta_list and free the associated memory with
 * both the Ta we are removing and the current student (if any).
 * Return 0 on success or 1 if this ta_name is not found in the list
 */
int remove_ta(Ta **ta_list_ptr, char *ta_name) {
    Ta *head = *ta_list_ptr;
    if (head == NULL) {
        return 1;
    } else if (strcmp(head->name, ta_name) == 0) {
        // TA is at the head so special case
        *ta_list_ptr = head->next;
        release_current_student(head);
        // memory for the student has been freed. Now free memory for the TA.
        free(head->name);
        free(head);
        return 0;
    }
    while (head->next != NULL) {
        if (strcmp(head->next->name, ta_name) == 0) {
            Ta *ta_tofree = head->next;
            //  We have found the ta to remove, but before we do that 
            //  we need to finish with the student and free the student.
            //  You need to complete this helper function
            release_current_student(ta_tofree);

            head->next = head->next->next;
            // memory for the student has been freed. Now free memory for the TA.
            free(ta_tofree->name);
            free(ta_tofree);
            return 0;
        }
        head = head->next;
    }
    // if we reach here, the ta_name was not in the list
    return 1;
}






/* TA ta_name is finished with the student they are currently helping (if any)
 * and are assigned to the next student in the full queue. 
 * If the queue is empty, then TA ta_name simply finishes with the student 
 * they are currently helping, records appropriate statistics, 
 * and sets current_student for this TA to NULL.
 * If ta_name is not in ta_list, return 1 and do nothing.
 */
int take_next_overall(char *ta_name, Ta *ta_list, Student **stu_list_ptr) {
    Ta *ta;
    if( (ta = find_ta(ta_list, ta_name)) == NULL){
        return 1;
    }
    release_current_student(ta);
    if(*stu_list_ptr == NULL){
        ta -> current_student = NULL;
    }else{
        Student *stu = *stu_list_ptr;
        ta -> current_student = stu;

        //update course info
        Course *course = stu -> course;
        course -> wait_time += time(NULL) - *(stu -> arrival_time);
        course -> head = stu -> next_course;
        if(course -> head == NULL){
            course -> tail = NULL;
        }

        //update general list
        *stu_list_ptr = stu -> next_overall;

        //update student info
        *(stu -> arrival_time) = time(NULL);
        stu -> next_overall = NULL;
        stu -> next_course = NULL;
    }
    return 0;
}



/* TA ta_name is finished with the student they are currently helping (if any)
 * and are assigned to the next student in the course with this course_code. 
 * If no student is waiting for this course, then TA ta_name simply finishes 
 * with the student they are currently helping, records appropriate statistics,
 * and sets current_student for this TA to NULL.
 * If ta_name is not in ta_list, return 1 and do nothing.
 * If course is invalid return 2, but finish with any current student. 
 */
int take_next_course(char *ta_name, Ta *ta_list, Student **stu_list_ptr, char *course_code, Course *courses, int num_courses) {
    Ta *ta;
    if( (ta = find_ta(ta_list, ta_name)) == NULL){
        return 1;
    }
    release_current_student(ta);
    
    Course *course = find_course(courses, num_courses, course_code);
    if(course == NULL){
        ta -> current_student = NULL;
        return 2;
    }
    
    if(course -> head == NULL){
        ta -> current_student = NULL;
    }else{
        Student *stu = course -> head;
        ta -> current_student = stu;

        //update course info
        course -> wait_time += time(NULL) - *(stu -> arrival_time);
        course -> head = stu -> next_course;
        if(course -> head == NULL){
            course -> tail = NULL;
        }

        //update general list
        if(stu == *stu_list_ptr){
            *stu_list_ptr = stu -> next_overall;
        }else{
            Student *stu_head = *stu_list_ptr;
            while(stu_head -> next_overall != stu){
                stu_head = stu_head -> next_overall;
            }
            stu_head -> next_overall = stu -> next_overall;
        }

        //update student info
        *(stu -> arrival_time) = time(NULL);
        stu -> next_overall = NULL;
        stu -> next_course = NULL;
    }
    return 0;
}

/* Return the number of students waiting in queue for a certain course.
*/
int student_waiting(Course *course){
    int total = 0;
    Student *head = course -> head;
    while(head != NULL){
        total += 1;
        head = head -> next_course;
    }
    return total;
}

/* For each course (in the same order as in the config file), print
 * the <course code>: <number of students waiting> "in queue\n" followed by
 * one line per student waiting with the format "\t%s\n" (tab name newline)
 * Uncomment and use the printf statements below. Only change the variable
 * names.
 */
void print_all_queues(Student *stu_list, Course *courses, int num_courses) {
    for(int i = 0; i< num_courses; i++){
        printf("%s: %d in queue\n", courses[i].code, student_waiting(courses + i) );
        Student *head = courses[i].head;
        while(head != NULL){
            printf("\t%s\n", head -> name);
            head = head -> next_course;
        }
    } 
}


/*
 * Print to stdout, a list of each TA, who they are serving at from what course
 * Uncomment and use the printf statements 
 */
void print_currently_serving(Ta *ta_list) {
    if(ta_list == NULL){
        printf("No TAs are in the help centre.\n");
    }else{
        Ta *head = ta_list;
        while(head != NULL){
            if(head -> current_student != NULL){
                printf("TA: %s is serving %s from %s\n", head->name, head -> current_student -> name, head -> current_student -> course -> code);
            }else{
                printf("TA: %s has no student\n", head -> name);
            }
            head = head -> next;
        }
    }
}


/*  list all students in queue (for testing and debugging)
 *   maybe suggest it is useful for debugging but not included in marking? 
 */ 
void print_full_queue(Student *stu_list) {
    Student *stu = stu_list;
    while(stu != NULL){
        printf("%s %s\n", stu -> name, stu -> course -> code);
        if(stu -> next_overall != NULL){
            printf("Next overall: %s \n", stu -> next_overall -> name);
        }
        if(stu -> next_course != NULL){
            printf("Next course: %s \n", stu -> next_course -> name);
        }
        stu = stu -> next_overall;
    }
}

/* Return the number of students who is being helped with a certain course.
*/
int count_being_helped(Ta *ta_list, Course *course){
    int result = 0;
    Ta *head = ta_list;
    while(head != NULL){
        if(head -> current_student != NULL){
            if(head -> current_student -> course == course){
                result += 1;
            }
        }
        head = head -> next;
    }
    return result;
}

/* Prints statistics to stdout for course with this course_code
 * See example output from assignment handout for formatting.
 *
 */
int stats_by_course(Student *stu_list, char *course_code, Course *courses, int num_courses, Ta *ta_list) {

    // TODO: students will complete these next pieces but not all of this 
    //       function since we want to provide the formatting
    Course *found;
    if ( (found = find_course(courses, num_courses, course_code)) == NULL){
        return 1;
    }  
    // You MUST not change the following statements or your code 
    //  will fail the testing. 

    int students_waiting = student_waiting(found);
    int students_being_helped = count_being_helped(ta_list, found);
    printf("%s:%s \n", found->code, found->description);
    printf("\t%d: waiting\n", students_waiting);
    printf("\t%d: being helped currently\n", students_being_helped);
    printf("\t%d: already helped\n", found->helped);
    printf("\t%d: gave_up\n", found->bailed);
    printf("\t%f: total time waiting\n", found->wait_time);
    printf("\t%f: total time helping\n", found->help_time);
    
    return 0;
}


/* Dynamically allocate space for the array course list and populate it
 * according to information in the configuration file config_filename
 * Return the number of courses in the array.
 * If the configuration file can not be opened, call perror() and exit.
 */
int config_course_list(Course **courselist_ptr, char *config_filename) {
    FILE *fp;
    
    if( (fp = fopen(config_filename, "r")) == NULL){
        perror("Error opening config file");
        exit(1);
    }

    // number of courses
    int course_num;
    char str[INPUT_BUFFER_SIZE];
    fgets(str, INPUT_BUFFER_SIZE, fp);
    course_num = strtol(str, NULL, 10);

    *courselist_ptr = malloc(sizeof(Course) * course_num);
    if(*courselist_ptr == NULL){
        perror("malloc for courselist");
        exit(1);
    }
    for(int i = 0; i < course_num; i++){
        fgets(str, INPUT_BUFFER_SIZE, fp);

        //course code
        strncpy((*courselist_ptr)[i].code, str, 7);
        (*courselist_ptr)[i].code[6] = '\0';
        
        //course description
        (*courselist_ptr)[i].description = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
        if ( (*courselist_ptr)[i].description == NULL){
            perror("malloc for course description");
            exit(1);
        }
        strncpy((*courselist_ptr)[i].description, str + 7, INPUT_BUFFER_SIZE);
    }
    return course_num;
}
