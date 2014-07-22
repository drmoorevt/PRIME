function [indicesOfChange] = findStateDemarcations(data, startIndex, range)
  numProfiles  = length(data(1,:));
  for i = 1:numProfiles
      indicesOfChange(i) = findStateDemarcation(data(:,i), startIndex(i), range);
  end
end