function [indicesOfChange] = findStateDemarcations(data, startIndex)
  numProfiles  = length(data(1,:,4));
  for i = 1:numProfiles
      indicesOfChange(i) = findStateDemarcation(data, startIndex(i));
  end
end