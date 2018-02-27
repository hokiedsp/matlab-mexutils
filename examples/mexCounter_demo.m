%mexCounter_demo
%   This script (and mexCounter.cpp) demostrates the use of mexObjectHandle template class.
%   mexCounter() is a MEX function with an integer state, which gets incremented everytime
%   mexCounter() is called

clear all

% create 2 mexCounter objects. counters are uint64 scalars, each representing the address of
% where the mexCounter state is stored.
counter1 = mexCounter();
counter2 = mexCounter();

% Immediately create onCleanup objects to auto-delete the mexObjectHandle that obj points to.
% Use of onCleanup is highly recommended as the anonymous function captures the copies of 
% counter1 and counter2. So, it makes sure that the mexObjectHandles will be deleted even if 
% values of counter1 & counter2 are changed.
C = [onCleanup(@()mexCounter(counter1,'delete'))
     onCleanup(@()mexCounter(counter2,'delete'))];

% increment the first counter 10 times
fprintf('counter1: ');
for n = 1:10
  fprintf('%d ', mexCounter(counter1));
end
fprintf('\n');

% increment the second counter 5 times
fprintf('counter2: ');
for n = 1:5
  fprintf('%d ', mexCounter(counter2));
end
fprintf('\n');

% increment the first counter again just to see
fprintf('counter1: %d\n', mexCounter(counter1));

% "accidentally" change counter1
counter1 = 'shouldn''t do this';

% when onCleanup object array is cleared, both mexObjectHandle objects get destroyed
clear C

% when the mexObjectHandle is deleted, it is a good practice to clear the pointer
counter2 = [];
